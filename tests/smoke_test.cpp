#include "pdi/cli.hpp"
#include "pdi/contract.hpp"
#include "pdi/operations.hpp"

#include <cassert>

int main()
{
    char a0[] = "pdi_lab";
    char a1[] = "--operation";
    char a2[] = "negative";
    char a3[] = "--input";
    char a4[] = "images/input/m1_gray_ramp_256.png";
    char a5[] = "--output";
    char a6[] = "images/output/out.png";
    char* argv[] = {a0, a1, a2, a3, a4, a5, a6};

    const auto options = pdi::parse_cli(7, argv);
    assert(options.operation == "negative");
    assert(pdi::is_known_operation(options.operation));
    assert(!pdi::is_known_operation("x"));
    assert(pdi::validate_contract(options));

    return 0;
}
