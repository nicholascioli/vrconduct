#include <score.hpp>

#include <memory>
#include <stdexcept>

#include <iostream>
#include <fstream>

//##############################################################################
// Channel
//##############################################################################
score::channel::channel(tml_message* m, const std::string& sfp)
: m_start{m}, m_current{m}, m_end{m}, m_renderer{tsf_load_filename(sfp.c_str())}, m_offset{0}, m_time{0.0} {
	if (!m_renderer)
		throw std::runtime_error("Could not load SoundFont: " + sfp);

	tsf_set_output(m_renderer, TSF_MONO, 44100, 0);
}

score::channel::~channel() {
	if (m_renderer)
		tsf_close(m_renderer);
}

score::channel::channel(channel&& other)
: m_start{other.m_start}, m_current{other.m_current}, m_renderer{other.m_renderer}, m_time{0} {
	other.m_renderer = nullptr;
}

score::channel& score::channel::operator =(channel&& other) {
	if (m_renderer)
		tsf_close(m_renderer);

	m_start    = other.m_start;
	m_current  = other.m_current;
	m_renderer = other.m_renderer;

	other.m_renderer = nullptr;
	return *this;
}

void score::channel::push_event(tml_message* m) {
	m_end->next = m;
	m_end = m;
}

const tml_message* score::channel::peek_event() const {
	return m_current;
}

tml_message* score::channel::pop_event() {
	auto* m = m_current;
	if (m_current)
		m_current = m_current->next;

	return m;
}

// We return true in order to trick the importer to allow us to stream
bool score::channel::isFile() const {
	return true;
}

size_t score::channel::read(void* buf, size_t count) {
	// std::cout << "Start reading... (" << count << ")" << std::endl;
	std::vector<channel_sample> d(count / sizeof(channel_sample));

	auto* lmf = m_current;
	auto* lsf = m_renderer;
	auto delta = d.size() * (1000.0 / (double)AUDIO_FREQ) / 4;

	for (auto i = 0u; i != 4; ++i) {
		while (lmf && lmf->time < m_time + delta) {
			switch (lmf->type) {
				case TML_PROGRAM_CHANGE:
					tsf_channel_set_presetnumber(lsf, lmf->channel, lmf->program, (lmf->channel == 9));
					break;
				case TML_NOTE_ON:
					tsf_channel_note_on(lsf, lmf->channel, lmf->key, lmf->velocity / 127.f);
					break;
				case TML_NOTE_OFF:
					tsf_channel_note_off(lsf, lmf->channel, lmf->key);
					break;
				case TML_PITCH_BEND:
					tsf_channel_set_pitchwheel(lsf, lmf->channel, lmf->pitch_bend);
					break;
				case TML_CONTROL_CHANGE:
					tsf_channel_midi_control(lsf, lmf->channel, lmf->control, lmf->control_value);
					break;
				default:
					throw std::runtime_error("UNREACHABLE");
			}

			lmf = lmf->next;
		}

		m_current = lmf;
		m_offset += count;
		m_time += delta;

		tsf_render_short(lsf, d.data() + i * d.size() / 4, d.size() / 4, 0);
	}

	std::copy(d.begin(), d.end(), (channel_sample*)buf);
	// std::cout << "Read: " << d.size() * sizeof(channel_sample) << std::endl;
	return d.size() * sizeof(channel_sample);
}

void score::channel::skip(size_t count) {
	m_offset += count;
	m_time = (m_offset / sizeof(channel_sample)) * (1000.0 / AUDIO_FREQ);

	if (count < 0)
		m_current = m_start;
	
	while (m_current && m_current->time < m_time)
		m_current = m_current->next;
}

void score::channel::seek(size_t pos) {
	m_offset = pos;
	double t = (pos / sizeof(channel_sample)) * (1000.0 / AUDIO_FREQ);
	// std::cout << "SEEK: " << pos << ":" << m_time << std::endl;

	if (!m_current || t < m_time) {
		return;
		m_current = m_start;
	}
	
	m_time = t;
	while (m_current && m_current->time < m_time)
		m_current = m_current->next;
	
	short _[score::AUDIO_SIZE];
	// tsf_channel_note_off_all(m_renderer, m_start->channel);
	tsf_render_short(m_renderer, _, score::AUDIO_SIZE, 0);
}

size_t score::channel::tell() const {
	return m_offset;
}

bool score::channel::eof() const {
	return (m_current == nullptr);
}

bs::SPtr<bs::DataStream> score::channel::clone(bool copyData) const {
	throw std::runtime_error("No copying of stream allowed");
	// auto clone = std::make_shared<channel>(m_start);

	// if (copyData) {
	// 	clone->m_current = m_current;
	// 	clone->m_offset  = m_offset;
	// }

	// return clone;
}

void score::channel::close() {
	//
}

//##############################################################################
// Score
//##############################################################################
score::score(const std::string& midi_path, const std::string& soundfont_path) {
	tml_message* mf = m_messages = tml_load_filename(midi_path.c_str());
	if (!mf)
		throw std::runtime_error("Could not load MIDI file: " + midi_path);

	while (mf) {
		channel_index chi(mf->channel);
		if (!m_channels.count(chi))
			m_channels.try_emplace(chi, std::make_shared<channel>(mf, soundfont_path));
		else
			m_channels.at(chi)->push_event(mf);

		mf = mf->next;
	}
}

score::~score() {
	if (m_messages)
		tml_free(m_messages);
}

score::channel::ptr score::get_channel(channel_index i) {
	return m_channels.at(i);
}

std::map<score::channel_index, score::channel::ptr> score::get_channels() {
	return m_channels;
}
