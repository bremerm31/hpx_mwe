//  Copyright (c) 2014-2016 Hartmut Kaiser
//  Copyright (c)      2016 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_main.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/serialization.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct A
  : hpx::components::migration_support<
        hpx::components::component_base<A>
    >
{
    typedef hpx::components::migration_support<
            hpx::components::component_base<A>
        > base_type;

    A()=default;
    explicit A(int data) : dataA_(data) {}
    virtual ~A() {}

    hpx::id_type call() const
    {
        HPX_TEST(pin_count() != 0);
        return hpx::find_here();
    }

    void busy_work() const
    {
        HPX_TEST(pin_count() != 0);
        hpx::this_thread::sleep_for(std::chrono::seconds(1));
        HPX_TEST(pin_count() != 0);
    }

    hpx::future<void> lazy_busy_work() const
    {
        HPX_TEST(pin_count() != 0);

        auto f = hpx::make_ready_future_after(std::chrono::seconds(1));

        return f.then(
            [this](hpx::future<void> && f)
            {
                f.get();
                HPX_TEST(pin_count() != 0);
            });
    }

    virtual int get_data() const
    {
        HPX_TEST(pin_count() != 0);
        return dataA_;
    }

    int get_data_nonvirt() const { return get_data(); }

    virtual hpx::future<int> lazy_get_data() const
    {
        HPX_TEST(pin_count() != 0);

        auto f =
            hpx::make_ready_future(/*_after(std::chrono::seconds(1), */dataA_);

        return f.then(
            [this](hpx::future<int> && f)
            {
                HPX_TEST(pin_count() != 0);
                return f.get();
            });
    }
    hpx::future<int> lazy_get_data_nonvirt() const { return lazy_get_data(); }

    // Components which should be migrated using hpx::migrate<> need to
    // be Serializable and CopyConstructable. Components can be
    // MoveConstructable in which case the serialized data is moved into the
    // component's constructor.
    A(A const& rhs)
      : base_type(rhs), dataA_(rhs.dataA_)
    {}

    A(A && rhs)
      : base_type(std::move(rhs)), dataA_(rhs.dataA_)
    {}

    A& operator=(A const & rhs)
    {
        dataA_ = rhs.dataA_;
        return *this;
    }
    A& operator=(A && rhs)
    {
        dataA_ = rhs.dataA_;
        return *this;
    }

    HPX_DEFINE_COMPONENT_ACTION(A, call, call_action);
    HPX_DEFINE_COMPONENT_ACTION(A, busy_work, busy_work_action);
    HPX_DEFINE_COMPONENT_ACTION(A, lazy_busy_work, lazy_busy_work_action);
    HPX_DEFINE_COMPONENT_ACTION(A, get_data_nonvirt, get_data_action);
    HPX_DEFINE_COMPONENT_ACTION(A, lazy_get_data_nonvirt, lazy_get_data_action);

    template <typename Archive>
    void serialize(Archive& ar, unsigned version)
    {
        ar & dataA_;
    }
    HPX_SERIALIZATION_POLYMORPHIC(A);

protected:
    int dataA_ = 0;
};

typedef hpx::components::simple_component<A> server_type;
HPX_REGISTER_COMPONENT(server_type, A);

typedef A::call_action call_action;
HPX_REGISTER_ACTION_DECLARATION(call_action);
HPX_REGISTER_ACTION(call_action);

typedef A::busy_work_action busy_work_action;
HPX_REGISTER_ACTION_DECLARATION(busy_work_action);
HPX_REGISTER_ACTION(busy_work_action);

typedef A::lazy_busy_work_action lazy_busy_work_action;
HPX_REGISTER_ACTION_DECLARATION(lazy_busy_work_action);
HPX_REGISTER_ACTION(lazy_busy_work_action);

typedef A::get_data_action get_data_action;
HPX_REGISTER_ACTION_DECLARATION(get_data_action);
HPX_REGISTER_ACTION(get_data_action);

typedef A::lazy_get_data_action lazy_get_data_action;
HPX_REGISTER_ACTION_DECLARATION(lazy_get_data_action);
HPX_REGISTER_ACTION(lazy_get_data_action);

struct B : A, hpx::components::component_base<B>
{
    using wrapping_type = hpx::components::component_base<B>::wrapping_type;
    using wrapped_type  = hpx::components::component_base<B>::wrapped_type;
//    using hpx::components::simple_component_base<B>::set_back_ptr;
//    using hpx::components::simple_component_base<B>::finalize;

    using type_holder = B;
    using base_type_holder = A;

    B()=default;
    explicit B(int data) : dataB_(data) {}
    virtual ~B() {}

    virtual int get_data()
    {
        HPX_TEST(pin_count() != 0);
        return dataB_;
    }

    virtual hpx::future<int> lazy_get_data()
    {
        HPX_TEST(pin_count() != 0);

        auto f =
            hpx::make_ready_future(/*_after(std::chrono::seconds(1), */dataB_);

        return f.then(
            [this](hpx::future<int> && f)
            {
                HPX_TEST(pin_count() != 0);
                return f.get();
            });
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & hpx::serialization::base_object<A>(*this);
        ar & dataB_;
    }
    HPX_SERIALIZATION_POLYMORPHIC(B);
protected:
    int dataB_=0;
};

typedef hpx::components::simple_component<B> serverB_type;
HPX_REGISTER_DERIVED_COMPONENT_FACTORY(serverB_type, B, "A");

struct clientA
  : hpx::components::client_base<clientA, A>
{
    typedef hpx::components::client_base<clientA, A>
        base_type;

    clientA() {}
    clientA(hpx::shared_future<hpx::id_type> const& id) : base_type(id) {}
    clientA(hpx::id_type && id) : base_type(std::move(id)) {}

    hpx::id_type call() const
    {
        return call_action()(this->get_id());
    }

    hpx::future<void> busy_work() const
    {
        return hpx::async<busy_work_action>(this->get_id());
    }

    hpx::future<void> lazy_busy_work() const
    {
        return hpx::async<lazy_busy_work_action>(this->get_id());
    }

    int get_data() const
    {
        return get_data_action()(this->get_id());
    }

    int lazy_get_data() const
    {
        return lazy_get_data_action()(this->get_id()).get();
    }
};

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_component(hpx::id_type source, hpx::id_type target)
{
    // create component on given locality
    clientA t1(hpx::components::new_<B>(source,42));
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    try {
        // migrate t1 to the target
        clientA t2(hpx::components::migrate(t1, target));

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.get_data(), 42);
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_component(hpx::id_type source, hpx::id_type target)
{
    // create component on given locality
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_data(), 42);

    try {
        // migrate t1 to the target
        clientA t2(hpx::components::migrate(t1, target));

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.lazy_get_data(), 42);
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_busy_component(hpx::id_type source, hpx::id_type target)
{
    // create component on given locality
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    // add some concurrent busy work
    hpx::future<void> busy_work = t1.busy_work();

    try {
        // migrate t1 to the target
        clientA t2(hpx::components::migrate(t1, target));

        HPX_TEST_EQ(t1.get_data(), 42);

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.get_data(), 42);
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    // the busy work should be finished by now, wait anyways
    busy_work.wait();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_lazy_busy_component(hpx::id_type source, hpx::id_type target)
{
    // create component on given locality
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_data(), 42);

    // add some concurrent busy work
    hpx::future<void> lazy_busy_work = t1.lazy_busy_work();

    try {
        // migrate t1 to the target
        clientA t2(hpx::components::migrate(t1, target));

        HPX_TEST_EQ(t1.lazy_get_data(), 42);

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.lazy_get_data(), 42);
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    // the busy work should be finished by now, wait anyways
    lazy_busy_work.wait();

    return true;
}

bool test_migrate_component2(hpx::id_type source, hpx::id_type target)
{
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    std::size_t N = 100;

    try {
        // migrate an object back and forth between 2 localities a couple of
        // times
        for(std::size_t i = 0; i < N; ++i)
        {
            // migrate t1 to the target (loc2)
            clientA t2(hpx::components::migrate(t1, target));

            HPX_TEST_EQ(t1.get_data(), 42);

            // wait for migration to be done
            HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

            // the migrated object should have the same id as before
            HPX_TEST_EQ(t1.get_id(), t2.get_id());

            // the migrated object should live on the target now
            HPX_TEST_EQ(t2.call(), target);
            HPX_TEST_EQ(t2.get_data(), 42);

            hpx::cout << "." << std::flush;

            std::swap(source, target);
        }

        hpx::cout << std::endl;
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_component2(hpx::id_type source, hpx::id_type target)
{
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_data(), 42);

    std::size_t N = 100;

    try {
        // migrate an object back and forth between 2 localities a couple of
        // times
        for(std::size_t i = 0; i < N; ++i)
        {
            // migrate t1 to the target (loc2)
            clientA t2(hpx::components::migrate(t1, target));

            HPX_TEST_EQ(t1.lazy_get_data(), 42);

            // wait for migration to be done
            HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

            // the migrated object should have the same id as before
            HPX_TEST_EQ(t1.get_id(), t2.get_id());

            // the migrated object should live on the target now
            HPX_TEST_EQ(t2.call(), target);
            HPX_TEST_EQ(t2.lazy_get_data(), 42);

            hpx::cout << "." << std::flush;

            std::swap(source, target);
        }

        hpx::cout << std::endl;
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_busy_component2(hpx::id_type source, hpx::id_type target)
{
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    std::size_t N = 100;

    // First, spawn a thread (locally) that will migrate the object between
    // source and target a few times
    hpx::future<void> migrate_future = hpx::async(
        [source, target, t1, N]() mutable
        {
            for(std::size_t i = 0; i < N; ++i)
            {
                // migrate t1 to the target (loc2)
                clientA t2(hpx::components::migrate(t1, target));

                HPX_TEST_EQ(t1.get_data(), 42);

                // wait for migration to be done
                HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

                // the migrated object should have the same id as before
                HPX_TEST_EQ(t1.get_id(), t2.get_id());

                // the migrated object should live on the target now
                HPX_TEST_EQ(t2.call(), target);
                HPX_TEST_EQ(t2.get_data(), 42);

                hpx::cout << "." << std::flush;

                std::swap(source, target);
            }

            hpx::cout << std::endl;
        }
    );

    // Second, we generate tons of work which should automatically follow
    // the migration.
    hpx::future<void> create_work = hpx::async(
        [t1, N]()
        {
            for(std::size_t i = 0; i < 2*N; ++i)
            {
                hpx::cout
                    << hpx::naming::get_locality_id_from_id(t1.call())
                    << std::flush;
                HPX_TEST_EQ(t1.get_data(), 42);
            }
        }
    );

    hpx::wait_all(migrate_future, create_work);

    // rethrow exceptions
    try {
        migrate_future.get();
        create_work.get();
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_busy_component2(hpx::id_type source, hpx::id_type target)
{
    clientA t1 = hpx::new_<clientA>(source, 42);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    std::size_t N = 100;

    // First, spawn a thread (locally) that will migrate the object between
    // source and target a few times
    hpx::future<void> migrate_future = hpx::async(
        [source, target, t1, N]() mutable
        {
            for(std::size_t i = 0; i < N; ++i)
            {
                // migrate t1 to the target (loc2)
                clientA t2(hpx::components::migrate(t1, target));

                HPX_TEST_EQ(t1.lazy_get_data(), 42);

                // wait for migration to be done
                HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

                // the migrated object should have the same id as before
                HPX_TEST_EQ(t1.get_id(), t2.get_id());

                // the migrated object should live on the target now
                HPX_TEST_EQ(t2.call(), target);
                HPX_TEST_EQ(t2.lazy_get_data(), 42);

                hpx::cout << "." << std::flush;

                std::swap(source, target);
            }

            hpx::cout << std::endl;
        }
    );

    // Second, we generate tons of work which should automatically follow
    // the migration.
    hpx::future<void> create_work = hpx::async(
        [t1, N]()
        {
            for(std::size_t i = 0; i < 2*N; ++i)
            {
                hpx::cout
                    << hpx::naming::get_locality_id_from_id(t1.call())
                    << std::flush;
                HPX_TEST_EQ(t1.lazy_get_data(), 42);
            }
        }
    );

    hpx::wait_all(migrate_future, create_work);

    // rethrow exceptions
    try {
        migrate_future.get();
        create_work.get();
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_remote_localities();

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_component(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component(id, hpx::find_here()));
    }
    /*
    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_busy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_busy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_busy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_busy_component(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_busy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_busy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component(id, hpx::find_here()));
    }

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_component2(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component2(id, hpx::find_here()));
    }

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_busy_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_busy_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_busy_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_busy_component2(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_busy_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_busy_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component2(id, hpx::find_here()));
    }
*/
    return hpx::util::report_errors();
}

