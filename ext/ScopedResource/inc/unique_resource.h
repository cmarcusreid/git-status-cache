#ifndef UNIQUE_RESOURCE_H_
#define UNIQUE_RESOURCE_H_

namespace std{	namespace experimental{

enum class invoke_it { once, again };

template<typename R, typename D>
class unique_resource_t {
	R resource;
	D deleter;
	bool execute_on_destruction; // exposition only
	unique_resource_t& operator=(unique_resource_t const &) = delete;
	unique_resource_t(unique_resource_t const &) = delete; // no copies!

public:
	// construction
	explicit unique_resource_t(R && resource, D && deleter, bool shouldrun = true) noexcept
		: resource(std::move(resource))
		, deleter(std::move(deleter))
		, execute_on_destruction{ shouldrun }
	{ }

	// move
	unique_resource_t(unique_resource_t &&other) noexcept
	: resource(std::move(other.resource))
	, deleter(std::move(other.deleter))
	, execute_on_destruction{ other.execute_on_destruction }
	{
		other.release();
	}

	unique_resource_t& operator=(unique_resource_t &&other) noexcept
	{
		this->invoke(invoke_it::once);
		deleter = std::move(other.deleter);
		resource = std::move(other.resource);
		execute_on_destruction = other.execute_on_destruction;
		other.release();
		return *this;
	}

		// resource release
	~unique_resource_t()
	{
		this->invoke(invoke_it::once);
	}

	void invoke(invoke_it const strategy = invoke_it::once) noexcept
	{
		if (execute_on_destruction)
		{
			try
			{
				this->get_deleter()(resource);
			}
			catch (...)
			{
			}
		}
		execute_on_destruction = strategy == invoke_it::again;
	}

	R const & release() noexcept
	{
		execute_on_destruction = false;
		return this->get();
	}

	void reset(R && newresource) noexcept
	{
		invoke(invoke_it::again);
		resource = std::move(newresource);
	}

	// resource access
	R& get() noexcept
	{
		return resource;
	}

	operator R const &() const noexcept
	{
		return resource;
	}

	R operator->() const noexcept
	{
		return resource;
	}

	std::add_lvalue_reference_t<std::remove_pointer_t<R>> operator*() const
	{
		return *resource;
	}

	// deleter access
	const D & get_deleter() const noexcept
	{
		return deleter;
	}
};

//factories
template<typename R, typename D>
unique_resource_t<R, D> unique_resource(R && r, D t) noexcept
{
	return unique_resource_t<R, D>(std::move(r), std::move(t), true);
}

	template<typename R, typename D>
unique_resource_t<R, D> unique_resource_checked(R r, R invalid, D t) noexcept
{
	bool shouldrun = (r != invalid);
	return unique_resource_t<R, D>(std::move(r), std::move(t), shouldrun);
}

}}

#endif /* UNIQUE RESOURCE H */
