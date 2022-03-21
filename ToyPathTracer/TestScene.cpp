#include "TestScene.h"
#include <random>

inline float Random()
{
    return rand() / (RAND_MAX + 1.0);
}

inline float3 RandomVec3()
{
    return float3(Random(), Random(), Random());
}

TestScene::TestScene()
{
	//spheres.push_back({ 0, float3(0, -100.5, -1), 100 });
	//spheres.push_back({ 1, float3(0, 0, -1), 0.5});
	//spheres.push_back({ 2, float3(-1, 0, -1), 0.5 });
	//spheres.push_back({ 2, float3(-1, 0, -1), -0.45 });
	//spheres.push_back({ 3, float3(1, 0, -1), 0.5 });

	//materials.push_back({ 0, float3(0.8, 0.8, 0.0) });
	//materials.push_back({ 0, float3(0.1, 0.2, 0.5) });
	//materials.push_back({ 2, float3(0, 0, 0), 0, 1.5 });
	//materials.push_back({ 1, float3(0.8, 0.6, 0.2), 0 });

    int id = 0;
    for (int i = -11; i < 11; ++i)
    {
        for (int j = -11; j < 11; ++j)
        {
            auto index = Random();
            float3 center(i + 0.9 * Random(), 0.2, j + 0.9 * Random());

            if (length(center - float3(4, 0.2, 0)) > 0.9)
            {
                if (index < 0.8)
                {   // diffuse
                    float3 albedo = RandomVec3() * RandomVec3();
                    materials.push_back({ 0, albedo });
                }
                else if (index < 0.95)
                {   // metal
                    float3 albedo = RandomVec3() * 0.5 + 0.5;
                    float fuzz = Random() * 0.5;
                    materials.push_back({ 1, albedo, fuzz });
                }
                else
                {   // glass
                    materials.push_back({ 2, float3(0, 0, 0), 0, 1.5 });
                }
                spheres.push_back({ id++, center, 0.2 });
            }
        }
    }

    materials.push_back({ 0, float3(0.5, 0.5, 0.5) });
	spheres.push_back({ id++, float3(0, -1000, 0), 1000 });
	
    materials.push_back({ 2, float3(0, 0, 0), 0, 1.5 });
    spheres.push_back({ id++, float3(0, 1, 0), 1 });
    
    materials.push_back({ 0, float3(0.4, 0.2, 0.1) });
    spheres.push_back({ id++, float3(-4, 1, 0), 1 });
    
    materials.push_back({ 1, float3(0.7, 0.6, 0.5), 0 });
    spheres.push_back({ id++, float3(4, 1, 0), 1 });
}

void TestScene::GetData(void* dataSpheres, void* dataMaterials)
{
	memcpy(dataSpheres, &spheres[0], spheres.size() * sizeof(Sphere));
	memcpy(dataMaterials, &materials[0], materials.size() * sizeof(Material));
}
