#include <cpr/cpr.h>
#include <iostream>

// test main
int main(int argc, char **argv)
{
    // Fetch the content at https://api.github.com/repos/whoshuu/cpr/contributors
    // and capture result in object 'r'
    cpr::Response r = cpr::Get(
        cpr::Url{"https://api.github.com/repos/whoshuu/cpr/contributors"},
        cpr::Authentication("user", "pass", cpr::AuthMode::BASIC),
        cpr::Parameters{{"anon", "true"}, {"key", "value"}});

    // print result items

    std::cout << r.status_code << std::endl;
    std::cout << r.header["content-type"] << std::endl;
    std::cout << r.text << std::endl;
    return 0;
}