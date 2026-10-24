#include <glm/gtc/constants.hpp>
#include <iostream>
#include <luma/scene/Transform.hpp>
int main() {
    luma::Transform t;
    t.setPosition({3, 2, 1});
    t.setRotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 0, 1)));
    t.setScale({2, 3, 4});
    auto p = t.matrix() * glm::vec4(1, 0, 0, 1);
    if (glm::length(glm::vec3(p) - glm::vec3(3, 4, 1)) > 0.0001f) {
        std::cerr << "FAIL: TRS expected (3,4,1), got (" << p.x << ',' << p.y << ',' << p.z
                  << ")\n";
        return 1;
    }
    std::cout << "PASS TRS\n";
}
