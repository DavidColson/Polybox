/*
 * Copyright 2010-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_ERROR_H_HEADER_GUARD
#	error "Must be included from bx/error!"
#endif // BX_ERROR_H_HEADER_GUARD

namespace bx
{
	inline Error::Error()
		: m_code(0)
	{
	}

	inline void Error::reset()
	{
		m_code = 0;
		m_msg.clear();
	}

	inline void Error::setError(ErrorResult _errorResult, const StringView& _msg)
	{
		BX_ASSERT(0 != _errorResult.code, "Invalid ErrorResult passed to setError!");

		if (!isOk() )
		{
			return;
		}

		m_code = _errorResult.code;
		m_msg  = _msg;
	}

	inline bool Error::isOk() const
	{
		return 0 == m_code;
	}

	inline ErrorResult Error::get() const
	{
		ErrorResult result = { m_code };
		return result;
	}

	inline const StringView& Error::getMessage() const
	{
		return m_msg;
	}

	inline bool Error::operator==(const ErrorResult& _rhs) const
	{
		return _rhs.code == m_code;
	}

	inline bool Error::operator!=(const ErrorResult& _rhs) const
	{
		return _rhs.code != m_code;
	}

	inline ErrorScope::ErrorScope(Error* _err, const StringView& _name)
		: m_err(_err)
		, m_name(_name)
	{
		BX_ASSERT(NULL != _err, "_err can't be NULL");
	}

	inline ErrorScope::~ErrorScope()
	{
		if (m_name.isEmpty() )
		{
			BX_ASSERT(m_err->isOk(), "Error: 0x%08x `%.*s`"
				, m_err->get().code
				, m_err->getMessage().getLength()
				, m_err->getMessage().getPtr()
				);
		}
		else
		{
			BX_ASSERT(m_err->isOk(), "Error: %.*s - 0x%08x `%.*s`"
				, m_name.getLength()
				, m_name.getPtr()
				, m_err->get().code
				, m_err->getMessage().getLength()
				, m_err->getMessage().getPtr()
				);
		}
	}

	inline const StringView& ErrorScope::getName() const
	{
		return m_name;
	}

} // namespace bx
