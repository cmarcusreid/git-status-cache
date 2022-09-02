#ifndef SCOPE_GUARD_H_
#define SCOPE_GUARD_H_

// modeled slightly after Andrew L. Sandoval's talk and article(s)
namespace std{ namespace experimental{

template <typename D>
struct scope_guard_t {
	// construction
	explicit scope_guard_t(D &&f) noexcept
		: deleter(std::move(f))
		, execute_on_destruction{ true }
	{ }

	// move
	scope_guard_t(scope_guard_t &&rhs) noexcept
		: deleter(std::move(rhs.deleter))
		, execute_on_destruction{ rhs.execute_on_destruction }
	{
		rhs.release();
	}

	// release
	~scope_guard_t()
	{
		if (execute_on_destruction)
		{
			try
			{
				deleter();
			}
			catch (...)
			{
			}
		}
	}

	void release() noexcept
	{
		execute_on_destruction = false;
	}

private:
	scope_guard_t(scope_guard_t const &) = delete;
	void operator=(scope_guard_t const &) = delete;
	scope_guard_t& operator=(scope_guard_t &&) = delete;
	D deleter;
	bool execute_on_destruction; // exposition only
};

// usage: auto guard=scope guard([] std::cout << "done...";);
template <typename D>
scope_guard_t<D> scope_guard(D && deleter) noexcept
{
	return scope_guard_t<D>(std::move(deleter)); // fails with curlies
}

}}
#endif /* SCOPE GUARD H */
