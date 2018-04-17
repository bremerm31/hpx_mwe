#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>

#include <tuple>

struct Component : public hpx::components::simple_component_base<Component> {
    std::tuple<> empty;
};

struct Client
    : public hpx::components::client_base<Client, Component> {
    using BaseType = hpx::components::client_base<Client,Component>;

    Client()=default;
    Client(hpx::future<hpx::id_type>&& id) : BaseType(std::move(id)) {}

};


struct A {
    static Client client;
};

Client A::client = Client();

HPX_REGISTER_COMPONENT(hpx::components::simple_component<Component>, Component);

int hpx_main(int argc, char* argv[]) {

    hpx::future<hpx::id_type> _id = hpx::new_<hpx::components::simple_component<Component>>(
        hpx::find_here());

    A::client = Client(std::move(_id));

    hpx::future<void> registration_future = A::client.register_as("thing");
    registration_future.get();
    return hpx::finalize();
}

int main(int argc, char* argv[]) {
    return hpx_main(argc,argv);
}