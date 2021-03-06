/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/sound/decoders/raw.cpp $
 * $Id: raw.cpp 48506 2010-04-04 10:24:31Z lordhoto $
 *
 */

#include "common/endian.h"
#include "common/stream.h"

#include "sound/audiostream.h"
#include "sound/mixer.h"
#include "sound/decoders/raw.h"

namespace Audio {

// This used to be an inline template function, but
// buggy template function handling in MSVC6 forced
// us to go with the macro approach. So far this is
// the only template function that MSVC6 seemed to
// compile incorrectly. Knock on wood.
#define READ_ENDIAN_SAMPLE(is16Bit, isUnsigned, ptr, isLE) \
	((is16Bit ? (isLE ? READ_LE_UINT16(ptr) : READ_BE_UINT16(ptr)) : (*ptr << 8)) ^ (isUnsigned ? 0x8000 : 0))

#pragma mark -
#pragma mark --- RawMemoryStream ---
#pragma mark -

/**
 * A simple raw audio stream, purely memory based. It operates on a single
 * block of data, which is passed to it upon creation.
 * Optionally supports looping the sound.
 *
 * Design note: This code tries to be as efficient as possible (without
 * resorting to assembly, that is). To this end, it is written as a template
 * class. This way the compiler can create optimized code for each special
 * case. This results in a total of 12 versions of the code being generated.
 */
template<bool is16Bit, bool isUnsigned, bool isLE>
class RawMemoryStream : public SeekableAudioStream {
protected:
	const byte *_ptr;
	const byte *_end;
	const int _rate;
	const byte *_origPtr;
	const DisposeAfterUse::Flag _disposeAfterUse;
	const Timestamp _playtime;
	const bool _isStereo;

public:
	RawMemoryStream(int rate, bool stereo, const byte *ptr, uint len, DisposeAfterUse::Flag autoFreeMemory)
	    : _ptr(ptr), _end(ptr+len), _rate(rate), _origPtr(ptr),
	      _disposeAfterUse(autoFreeMemory),
	      _playtime(0, len / (is16Bit ? 2 : 1) / (stereo ? 2 : 1), rate),
	      _isStereo(stereo) {
	}

	virtual ~RawMemoryStream() {
		if (_disposeAfterUse == DisposeAfterUse::YES)
			free(const_cast<byte *>(_origPtr));
	}

	int readBuffer(int16 *buffer, const int numSamples);

	bool isStereo() const			{ return _isStereo; }
	bool endOfData() const			{ return _ptr >= _end; }

	int getRate() const				{ return _rate; }
	bool seek(const Timestamp &where);
	Timestamp getLength() const { return _playtime; }
};

template<bool is16Bit, bool isUnsigned, bool isLE>
int RawMemoryStream<is16Bit, isUnsigned, isLE>::readBuffer(int16 *buffer, const int numSamples) {
	int samples = numSamples;
	while (samples > 0 && _ptr < _end) {
		int len = MIN(samples, (int)(_end - _ptr) / (is16Bit ? 2 : 1));
		samples -= len;
		do {
			*buffer++ = READ_ENDIAN_SAMPLE(is16Bit, isUnsigned, _ptr, isLE);
			_ptr += (is16Bit ? 2 : 1);
		} while (--len);
	}
	return numSamples-samples;
}

template<bool is16Bit, bool isUnsigned, bool isLE>
bool RawMemoryStream<is16Bit, isUnsigned, isLE>::seek(const Timestamp &where) {
	const uint8 *ptr = _origPtr + convertTimeToStreamPos(where, getRate(), isStereo()).totalNumberOfFrames() * (is16Bit ? 2 : 1);
	if (ptr > _end) {
		_ptr = _end;
		return false;
	} else if (ptr == _end) {
		_ptr = _end;
		return true;
	} else {
		_ptr = ptr;
		return true;
	}
}

#pragma mark -
#pragma mark --- RawStream ---
#pragma mark -

/**
 * This is a stream, which allows for playing raw PCM data from a stream.
 * It also features playback of multiple blocks from a given stream.
 */
template<bool is16Bit, bool isUnsigned, bool isLE>
class RawStream : public SeekableAudioStream {

// Allow backends to override buffer size
#ifdef CUSTOM_AUDIO_BUFFER_SIZE
	static const int32 BUFFER_SIZE = CUSTOM_AUDIO_BUFFER_SIZE;
#else
	static const int32 BUFFER_SIZE = 16384;
#endif

protected:
	byte *_buffer;                                 ///< Streaming buffer
	const byte *_ptr;                              ///< Pointer to current position in stream buffer
	const int _rate;                               ///< Sample rate of stream
	const bool _isStereo;                          ///< Whether this is an stereo stream

	Timestamp _playtime;                           ///< Calculated total play time
	Common::SeekableReadStream *_stream;           ///< Stream to read data from
	int32 _filePos;                                ///< Current position in stream
	int32 _diskLeft;                               ///< Samples left in stream in current block not yet read to buffer
	int32 _bufferLeft;                             ///< Samples left in buffer in current block
	const DisposeAfterUse::Flag _disposeAfterUse;  ///< Indicates whether the stream object should be deleted when this RawStream is destructed

	const RawStreamBlockList _blocks;              ///< Audio block list
	RawStreamBlockList::const_iterator _curBlock;  ///< Current audio block number
public:
	RawStream(int rate, bool stereo, DisposeAfterUse::Flag disposeStream, Common::SeekableReadStream *stream, const RawStreamBlockList &blocks)
		: _rate(rate), _isStereo(stereo), _playtime(0, rate), _stream(stream), _disposeAfterUse(disposeStream), _blocks(blocks), _curBlock(_blocks.begin()) {

		assert(_blocks.size() > 0);

		// Allocate streaming buffer
		if (is16Bit)
			_buffer = (byte *)malloc(BUFFER_SIZE * sizeof(int16));
		else
			_buffer = (byte *)malloc(BUFFER_SIZE * sizeof(byte));

		_ptr = _buffer;
		_bufferLeft = 0;

		// Set current buffer state, playing first block
		_filePos = _curBlock->pos;
		_diskLeft = _curBlock->len;

		// Add up length of all blocks in order to caluclate total play time
		int32 len = 0;
		for (RawStreamBlockList::const_iterator i = _blocks.begin(); i != _blocks.end(); ++i) {
			assert(i->len % (_isStereo ? 2 : 1) == 0);
			len += i->len;
		}

		_playtime = Timestamp(0, len / (_isStereo ? 2 : 1), rate);
	}


	virtual ~RawStream() {
		if (_disposeAfterUse == DisposeAfterUse::YES)
			delete _stream;

		free(_buffer);
	}

	int readBuffer(int16 *buffer, const int numSamples);

	bool isStereo() const           { return _isStereo; }
	bool endOfData() const          { return (_curBlock == _blocks.end()) && (_diskLeft == 0) && (_bufferLeft == 0); }

	int getRate() const         { return _rate; }
	Timestamp getLength() const { return _playtime; }

	bool seek(const Timestamp &where);
};

template<bool is16Bit, bool isUnsigned, bool isLE>
int RawStream<is16Bit, isUnsigned, isLE>::readBuffer(int16 *buffer, const int numSamples) {
	int oldPos = _stream->pos();
	bool restoreFilePosition = false;

	int samples = numSamples;

	while (samples > 0 && ((_diskLeft > 0 || _bufferLeft > 0) || _curBlock != _blocks.end())) {
		// Output samples in the buffer to the output
		int len = MIN<int>(samples, _bufferLeft);
		samples -= len;
		_bufferLeft -= len;

		while (len > 0) {
			*buffer++ = READ_ENDIAN_SAMPLE(is16Bit, isUnsigned, _ptr, isLE);
			_ptr += (is16Bit ? 2 : 1);
			len--;
		}

		// Have we now finished this block?  If so, read the next block
		if ((_bufferLeft == 0) && (_diskLeft == 0) && _curBlock != _blocks.end()) {
			// Next block
			++_curBlock;

			if (_curBlock != _blocks.end()) {
				_filePos = _curBlock->pos;
				_diskLeft = _curBlock->len;
			}
		}

		// Now read more data from disk if there is more to be read
		if (_bufferLeft == 0 && _diskLeft > 0) {
			int32 readAmount = MIN(_diskLeft, BUFFER_SIZE);

			// TODO: We should check for both seek and read to success.
			// If that is not the case, we should probably stop the
			// stream playback.
			_stream->seek(_filePos, SEEK_SET);
			_stream->read(_buffer, readAmount * (is16Bit ? 2 : 1));

			// Amount of data in buffer is now the amount read in, and
			// the amount left to read on disk is decreased by the same amount
			_bufferLeft = readAmount;
			_diskLeft -= readAmount;
			_ptr = (byte *)_buffer;
			_filePos += readAmount * (is16Bit ? 2 : 1);

			// Set this flag now we've used the file, it restores it's
			// original position.
			restoreFilePosition = true;
		}
	}

	// In case calling code relies on the position of this stream staying
	// constant, I restore the location if I've changed it.  This is probably
	// not necessary.
	if (restoreFilePosition)
		_stream->seek(oldPos, SEEK_SET);

	return numSamples - samples;
}

template<bool is16Bit, bool isUnsigned, bool isLE>
bool RawStream<is16Bit, isUnsigned, isLE>::seek(const Timestamp &where) {
	_filePos = 0;
	_diskLeft = 0;
	_bufferLeft = 0;
	_curBlock = _blocks.end();

	if (where > _playtime)
		return false;

	const uint32 seekSample = convertTimeToStreamPos(where, getRate(), isStereo()).totalNumberOfFrames();
	uint32 curSample = 0;

	// Search for the disk block in which the specific sample is placed
	for (_curBlock = _blocks.begin(); _curBlock != _blocks.end(); ++_curBlock) {
		uint32 nextBlockSample = curSample + _curBlock->len;

		if (nextBlockSample > seekSample)
			break;

		curSample = nextBlockSample;
	}

	if (_curBlock == _blocks.end()) {
		return ((seekSample - curSample) == 0);
	} else {
		const uint32 offset = seekSample - curSample;

		_filePos = _curBlock->pos + offset * (is16Bit ? 2 : 1);
		_diskLeft = _curBlock->len - offset;

		return true;
	}
}

#pragma mark -
#pragma mark --- Raw stream factories ---
#pragma mark -

/* In the following, we use preprocessor / macro tricks to simplify the code
 * which instantiates the input streams. We used to use template functions for
 * this, but MSVC6 / EVC 3-4 (used for WinCE builds) are extremely buggy when it
 * comes to this feature of C++... so as a compromise we use macros to cut down
 * on the (source) code duplication a bit.
 * So while normally macro tricks are said to make maintenance harder, in this
 * particular case it should actually help it :-)
 */

#define MAKE_RAW_STREAM(UNSIGNED) \
		if (is16Bit) { \
			if (isLE) \
				return new RawStream<true, UNSIGNED, true>(rate, isStereo, disposeAfterUse, stream, blockList); \
			else  \
				return new RawStream<true, UNSIGNED, false>(rate, isStereo, disposeAfterUse, stream, blockList); \
		} else \
			return new RawStream<false, UNSIGNED, false>(rate, isStereo, disposeAfterUse, stream, blockList)

SeekableAudioStream *makeRawStream(Common::SeekableReadStream *stream,
                                   const RawStreamBlockList &blockList,
                                   int rate,
                                   byte flags,
                                   DisposeAfterUse::Flag disposeAfterUse) {
	const bool isStereo   = (flags & Audio::FLAG_STEREO) != 0;
	const bool is16Bit    = (flags & Audio::FLAG_16BITS) != 0;
	const bool isUnsigned = (flags & Audio::FLAG_UNSIGNED) != 0;
	const bool isLE       = (flags & Audio::FLAG_LITTLE_ENDIAN) != 0;

	if (blockList.empty()) {
		warning("Empty block list passed to makeRawStream");
		if (disposeAfterUse == DisposeAfterUse::YES)
			delete stream;
		return 0;
	}

	if (isUnsigned) {
		MAKE_RAW_STREAM(true);
	} else {
		MAKE_RAW_STREAM(false);
	}
}

SeekableAudioStream *makeRawStream(Common::SeekableReadStream *stream,
                                   int rate, byte flags,
                                   DisposeAfterUse::Flag disposeAfterUse) {
	RawStreamBlockList blocks;
	RawStreamBlock block;
	block.pos = 0;

	const bool isStereo   = (flags & Audio::FLAG_STEREO) != 0;
	const bool is16Bit    = (flags & Audio::FLAG_16BITS) != 0;

	assert(stream->size() % ((is16Bit ? 2 : 1) * (isStereo ? 2 : 1)) == 0);

	block.len = stream->size() / (is16Bit ? 2 : 1);
	blocks.push_back(block);

	return makeRawStream(stream, blocks, rate, flags, disposeAfterUse);
}

#define MAKE_LINEAR(UNSIGNED) \
		if (is16Bit) { \
			if (isLE) \
				return new RawMemoryStream<true, UNSIGNED, true>(rate, isStereo, buffer, size, disposeAfterUse); \
			else  \
				return new RawMemoryStream<true, UNSIGNED, false>(rate, isStereo, buffer, size, disposeAfterUse); \
		} else \
			return new RawMemoryStream<false, UNSIGNED, false>(rate, isStereo, buffer, size, disposeAfterUse)

SeekableAudioStream *makeRawStream(const byte *buffer, uint32 size,
                                   int rate, byte flags,
                                   DisposeAfterUse::Flag disposeAfterUse) {
	const bool isStereo   = (flags & Audio::FLAG_STEREO) != 0;
	const bool is16Bit    = (flags & Audio::FLAG_16BITS) != 0;
	const bool isUnsigned = (flags & Audio::FLAG_UNSIGNED) != 0;
	const bool isLE       = (flags & Audio::FLAG_LITTLE_ENDIAN) != 0;

	// Verify the buffer sizes are sane
	if (is16Bit && isStereo) {
		assert((size & 3) == 0);
	} else if (is16Bit || isStereo) {
		assert((size & 1) == 0);
	}

	if (isUnsigned)
		MAKE_LINEAR(true);
	else
		MAKE_LINEAR(false);
}

SeekableAudioStream *makeRawDiskStream_OLD(Common::SeekableReadStream *stream, RawStreamBlock *block, int numBlocks,
                                           int rate, byte flags, DisposeAfterUse::Flag disposeStream) {
	assert(numBlocks > 0);
	RawStreamBlockList blocks;
	for (int i = 0; i < numBlocks; ++i)
		blocks.push_back(block[i]);

	return makeRawStream(stream, blocks, rate, flags, disposeStream);
}

} // End of namespace Audio
