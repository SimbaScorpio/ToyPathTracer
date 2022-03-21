#pragma once

#include<vector>

#include "Maths.h"
#include "SharedDataStruct.h"

class TestScene
{
public:
    TestScene();
    int GetSphereSize() { return spheres.size(); }
    int GetMaterialSize() { return materials.size(); }
    void GetData(void* spheres, void* materials);

private:
    std::vector<Sphere> spheres;
    std::vector<Material> materials;
};
