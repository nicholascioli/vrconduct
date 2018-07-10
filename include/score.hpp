#pragma once

#include <map>
#include <string>
#include <TinySoundFont/tsf.h>
#include <TinySoundFont/tml.h>

#include "FileSystem/BsDataStream.h"

class score {
public:
	static const size_t AUDIO_SIZE = 512;
	static const size_t AUDIO_FREQ = 44100;

public:
	using channel_index = size_t;
	using channel_sample = short;

	class channel : public bs::DataStream {
	public:
		using ptr = std::shared_ptr<channel>;

	public:
		channel(tml_message* m, const std::string& sfp);
		~channel();

		channel(const channel&) = delete;
		channel(channel&& other);

		channel& operator =(const channel&) = delete;
		channel& operator =(channel&& other);

	public:
		void push_event(tml_message* m);

		const tml_message* peek_event() const;
		tml_message* pop_event();

	public:
		//-------------------
		// Overrides
		//-------------------

		// Will always return true for streaming
		bool isFile() const;
		
		/**
		 * Read the requisite number of bytes from the stream, stopping at the end of the file. Advances
		 * the read pointer.
		 *
		 * @param[in]	buf		Pre-allocated buffer to read the data into.
		 * @param[in]	count	Number of bytes to read.
		 * @return				Number of bytes actually read.
		 * 			
		 * @note	Stream must be created with READ access mode.
		 */
		size_t read(void* buf, size_t count);
		
		/**
		 * Skip a defined number of bytes. This can also be a negative value, in which case the file pointer rewinds a 
		 * defined number of bytes.
		 */
		void skip(size_t count);

		/** Repositions the read point to a specified byte. */
		void seek(size_t pos);

		/** Returns the current byte offset from beginning. */
		size_t tell() const;

		/** Returns true if the stream has reached the end. */
		bool eof() const;

		/** 
		 * Creates a copy of this stream. 
		 *
		 * @param[in]	copyData	If true the internal stream data will be copied as well, otherwise it will just 
		 *							reference the data from the original stream (in which case the caller must ensure the
		*							original stream outlives the clone). This is not relevant for file streams.
		*/
		bs::SPtr<DataStream> clone(bool copyData = true) const;

		/** Close the stream. This makes further operations invalid. */
		void close();

	private:
		tml_message* m_start;
		tml_message* m_current;
		tml_message* m_end;

		tsf* m_renderer;

		size_t m_offset;
		double m_time;
	};

public:
	// score(const std::string& midi_path, ...);
	score(const std::string& midi_path, const std::string& soundfont_path);
	~score();

public:
	channel::ptr get_channel(channel_index i);
	std::map<channel_index, channel::ptr> get_channels();

private:
	std::map<channel_index, channel::ptr> m_channels;
	tml_message* m_messages;
};