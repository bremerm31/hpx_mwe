//  Copyright (c) 2014-2016 Hartmut Kaiser
//  Copyright (c)      2016 Thomas Heller
//  Copyright (c)      2018 Maximilian Bremer
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
#include <sstream>
#include <utility>
#include <vector>

HPX_REGISTER_CHANNEL(int);

///////////////////////////////////////////////////////////////////////////////
struct test_server
  : hpx::components::migration_support<
        hpx::components::component_base<test_server>
    >
{
    typedef hpx::components::migration_support<
            hpx::components::component_base<test_server>
        > base_type;
    test_server() = default;

    test_server(hpx::id_type id) : c(hpx::find_here()) {
        std::stringstream s;
        s << id;
        std::string channel_name{"foobar_"+s.str()};
        hpx::cout << "Registering channel under name: " << channel_name << hpx::endl;
        c.register_as(channel_name).get();
    }
    ~test_server() {}

    hpx::id_type call() const
    {
        return hpx::find_here();
    }

    void busy_work(int val)
    {
        hpx::this_thread::sleep_for(std::chrono::seconds(1));
        c.set(val);
    }

    int get_data() const
    {
        return c.get(hpx::launch::sync);
    }

    // Components which should be migrated using hpx::migrate<> need to
    // be Serializable and CopyConstructable. Components can be
    // MoveConstructable in which case the serialized data is moved into the
    // component's constructor.
    test_server(test_server const& rhs)
      : base_type(rhs), c(rhs.c)
    {}

    test_server(test_server && rhs)
      : base_type(std::move(rhs)), c(rhs.c)
    {}

    test_server& operator=(test_server const & rhs)
    {
        c = rhs.c;
        return *this;
    }
    test_server& operator=(test_server && rhs)
    {
        c = rhs.c;
        return *this;
    }

    HPX_DEFINE_COMPONENT_ACTION(test_server, call, call_action);
    HPX_DEFINE_COMPONENT_ACTION(test_server, busy_work, busy_work_action);
    HPX_DEFINE_COMPONENT_ACTION(test_server, get_data, get_data_action);

    template <typename Archive>
    void serialize(Archive& ar, unsigned version)
    {
        ar & c;
    }

private:
    hpx::lcos::channel<int> c;
};

typedef hpx::components::simple_component<test_server> server_type;
HPX_REGISTER_COMPONENT(server_type, test_server);

typedef test_server::call_action call_action;
HPX_REGISTER_ACTION_DECLARATION(call_action);
HPX_REGISTER_ACTION(call_action);

typedef test_server::busy_work_action busy_work_action;
HPX_REGISTER_ACTION_DECLARATION(busy_work_action);
HPX_REGISTER_ACTION(busy_work_action);

typedef test_server::get_data_action get_data_action;
HPX_REGISTER_ACTION_DECLARATION(get_data_action);
HPX_REGISTER_ACTION(get_data_action);

struct test_client
  : hpx::components::client_base<test_client, test_server>
{
    typedef hpx::components::client_base<test_client, test_server>
        base_type;

    test_client() {}
    test_client(hpx::shared_future<hpx::id_type> const& id) : base_type(id) {}
    test_client(hpx::id_type && id) : base_type(std::move(id)) {}

    hpx::id_type call() const
    {
        return call_action()(this->get_id());
    }

    hpx::future<void> busy_work(int val)
    {
        return hpx::async<busy_work_action>(this->get_id(),val);
    }

    int get_data() const
    {
        return get_data_action()(this->get_id());
    }
};

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_busy_component(hpx::id_type source, hpx::id_type target)
{
    hpx::lcos::channel<int> c2;
    {
        std::stringstream s;
        s << source;
        std::string channel_name{"foobar_"+s.str()};
        hpx::cout << "Connecting to channel: " << channel_name << hpx::endl;
        c2.connect_to("foobar");
    }

    // create component on given locality
    test_client t1 = hpx::new_<test_client>(source, source);
    HPX_TEST_NEQ(hpx::naming::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);

    // add some concurrent busy work
    hpx::future<void> busy_work = t1.busy_work(42).then([&t1](auto&& f) {
            return t1.busy_work(42);
            });

    try {
        // migrate t1 to the target
        test_client t2(hpx::components::migrate(t1, target));

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::naming::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);

        // the busy work should be finished by now, wait anyways
        busy_work.wait();

        HPX_TEST_EQ(t2.get_data(), 42); //get data from component

        if (false) {
            HPX_TEST_EQ(t2.get_data(), 42);
        } else {
            HPX_TEST_EQ(c2.get(hpx::launch::sync), 42); //get data from client connected to channel
        }
    }
    catch (hpx::exception const& e) {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::vector<hpx::id_type> localities = hpx::find_remote_localities();

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_busy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_busy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_busy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_busy_component(id, hpx::find_here()));
    }

    std::cout << hpx::util::report_errors() << std::endl;
    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main() {
    return hpx::init();
}