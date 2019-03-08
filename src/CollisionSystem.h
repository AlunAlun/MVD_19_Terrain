#pragma once
#include "includes.h"
#include "Components.h"

class CollisionSystem {
public:
    void init();
    void update(float dt);
    bool intersectSegmentBox(Collider& ray, Collider& box, lm::vec3& col_point, float& col_distance, float max_distance = 100000.0f);
    
    bool intersectSegmentTriangle(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c);
    bool intersectSegmentQuad(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c, lm::vec3 d, lm::vec3& r);
    
    //LINE not segment
    bool intersectLineQuad(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c, lm::vec3 d, lm::vec3& r);
};

