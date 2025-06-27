#include "ECS.h"

using namespace silk;

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health { int hp; };
struct Name { std::string value; };

int main()
{
    // addComponent()
    {
        Scene scene;
        Entity e = scene.createEntity();
        scene.addComponent(e, Health{5});
        assert(scene.hasComponent<Health>(e));
        auto health = scene.getComponent<Health>(e);
        assert(health.hp == 5);
    }
  
    // getComponent()
    {
        Scene scene;
        Entity e = scene.createEntity(Position{1, 2}, Velocity{3, 4});
        assert(scene.hasComponent<Position>(e));
        assert(scene.hasComponent<Velocity>(e));
        auto& pos = scene.getComponent<Position>(e);
        auto& vel = scene.getComponent<Velocity>(e);
        assert(pos.x == 1 && pos.y == 2);
        assert(vel.dx == 3 && vel.dy == 4);
    }

    // removeComponent()
    {
        Scene scene;
        Entity e = scene.createEntity(Position{10, 10}, Health{100});
        scene.removeComponent<Health>(e);
        assert(!scene.hasComponent<Health>(e));
        assert(scene.hasComponent<Position>(e));
    }

    // deleteEntity()
    {
        Scene scene;
        Entity e = scene.createEntity(Position{5, 5}, Velocity{0.1f, 0.2f});
        scene.deleteEntity(e);
        assert(!scene.hasComponent<Position>(e));
        assert(!scene.hasComponent<Velocity>(e));
    }

    // reuse from freedEntities
    {
        Scene scene;
        Entity e1 = scene.createEntity(Position{0, 0});
        scene.deleteEntity(e1);
        Entity e2 = scene.createEntity(Position{1, 1});
        assert(e1 == e2); // should reuse freed ID
    }

    // query()
    {
        Scene scene;
        Entity a = scene.createEntity(Position{1, 1}, Velocity{1, 0});
        Entity b = scene.createEntity(Position{2, 2});
        Entity c = scene.createEntity(Position{3, 3}, Velocity{0, 1});

        auto result = scene.query<Position, Velocity>();
        assert(result.size() == 2);
        assert((result[0] == a || result[0] == c));
        assert((result[1] == a || result[1] == c));
        assert(result[0] != result[1]);

        auto emptyResult = scene.query<Name>();
        assert(emptyResult.empty());
    }

    return 0;
}