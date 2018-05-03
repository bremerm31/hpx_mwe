#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/serialization.hpp>

#include <tuple>

HPX_REGISTER_CHANNEL(int);


int hpx_main(int argc, char* argv[]) {

    hpx::lcos::channel<int> c(hpx::find_here());
    c.set(1);


    //santity check (this works)
    /*if ( c.get().get() != 1 ) {
        std::cerr << "Error Values not equal\n";
    } */

    //sanity check #2 (this works)
    /*auto c3 = c;
    if ( c3.get().get() != 1 ) {
        std::cerr << "Error Values not equal\n";
        }*/

    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << c;

    hpx::serialization::input_archive iarchive(buffer);
    hpx::lcos::channel<int> c2;
    iarchive >> c2;

    if ( c2.get().get() != 1 ) {
        std::cerr << "Error Values not equal\n";
    }
    
    return hpx::finalize();
}

int main(int argc, char* argv[]) {
    return hpx_main(argc,argv);
}