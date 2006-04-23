#include "Stream.h"

#include "wx/mstream.h"
#include "wx/zstream.h"

namespace DatafileIO
{

	// Wrappers around wx streams, so they can be used as [non-wx] Streams.
	// (This code is all inline so that the project can be compiled without
	// requiring wxWidgets.)

	class SeekableInputStreamFromWx : public SeekableInputStream
	{
	public:
		// Take ownership of a wxInputStream
		SeekableInputStreamFromWx(wxInputStream* stream)
			: m_Stream(stream), m_Owner(true) {}

		// Use an externally-owned wxInputStream
		SeekableInputStreamFromWx(wxInputStream& stream)
			: m_Stream(&stream), m_Owner(false) {}

		~SeekableInputStreamFromWx()
		{
			if (m_Owner)
				delete m_Stream;
		}

		virtual off_t Tell() const { return m_Stream->TellI(); }
		virtual bool IsOk() const { return m_Stream->IsOk(); }

		virtual void Seek(off_t pos, Stream::whence mode)
		{
			m_Stream->SeekI(pos,
				mode==Stream::FROM_START ? wxFromStart
				: mode==Stream::FROM_END ? wxFromEnd
				: /*mode==Stream::FROM_CURRENT*/ wxFromCurrent);
		}

		virtual size_t Read(void* buffer, size_t size)
		{
			m_Stream->Read(buffer, size);
			return m_Stream->LastRead();
		}

	private:
		wxInputStream* m_Stream;
		bool m_Owner;
	};


	class SeekableOutputStreamFromWx : public SeekableOutputStream
	{
	public:
		// Take ownership of a wxInputStream
		SeekableOutputStreamFromWx(wxOutputStream* stream)
			: m_Stream(stream), m_Owner(true) {}

		// Use an externally-owned wxInputStream
		SeekableOutputStreamFromWx(wxOutputStream& stream)
			: m_Stream(&stream), m_Owner(false) {}

		~SeekableOutputStreamFromWx()
		{
			if (m_Owner)
				delete m_Stream;
		}

		virtual off_t Tell() const
		{
			return m_Stream->TellO();
		}

		virtual bool IsOk() const
		{
			return m_Stream->IsOk();
		}

		virtual void Seek(off_t pos, Stream::whence mode)
		{
			m_Stream->SeekO(pos,
				mode==Stream::FROM_START ? wxFromStart
				: mode==Stream::FROM_END ? wxFromEnd
				: /*mode==Stream::FROM_CURRENT*/ wxFromCurrent);
		}

		virtual void Write(const void* buffer, size_t size)
		{
			m_Stream->Write(buffer, size);
		}

	private:
		wxOutputStream* m_Stream;
		bool m_Owner;
	};

}
