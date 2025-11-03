//
// Created by Emma Clarke on 03/11/2025.
//

#include <dispatch/dispatch.h>
#include <SFML/Graphics.hpp>
#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Dynamics/b2World.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Vector2.hpp"
using namespace sf;
using namespace std;

b2World* world;
std::vector<b2Body*> bodies;
std::vector<RectangleShape*> sprites;

const int gameWidth = 800;
const int gameHeight = 600;
// 1 sfml unit = 30 physics units
const float physics_scale = 30.0f;
// inverse of physics_scale, useful for calculations
const float physics_scale_inv = 1.0f / physics_scale;
// Magic numbers for accuracy of physics simulation
const int32 velocityIterations = 6;
const int32 positionIterations = 2;

//Convert from b2Vec2 to a Vector2f
inline const Vector2f bv2_to_sv2(const b2Vec2& in) {
    return Vector2f(in.x * physics_scale, (in.y * physics_scale));
}
//Convert from Vector2f to a b2Vec2
inline const b2Vec2 sv2_to_bv2(const Vector2f& in) {
    return b2Vec2(in.x * physics_scale_inv, (in.y * physics_scale_inv));
}
//Convert from screenspace.y to physics.y (as they are the other way around)
inline const Vector2f invert_height(const Vector2f& in) {
    return Vector2f(in.x, gameHeight - in.y);
}

//Create a Box2D body with a box fixture
b2Body* CreatePhysicsBox(b2World& World, const bool dynamic, const Vector2f& position, const Vector2f& size) {
    b2BodyDef BodyDef;
    //Is Dynamic(moving), or static(Stationary)
    BodyDef.type = dynamic ? b2_dynamicBody : b2_staticBody;
    BodyDef.position = sv2_to_bv2(position);
    //Create the body
    b2Body* body = World.CreateBody(&BodyDef);

    //Create the fixture shape
    b2PolygonShape Shape;
    Shape.SetAsBox(sv2_to_bv2(size).x * 0.5f, sv2_to_bv2(size).y * 0.5f);
    b2FixtureDef FixtureDef;
    //Fixture properties
    FixtureDef.density = dynamic ? 10.f : 0.f;
    FixtureDef.friction = dynamic ? 0.8f : 1.f;
    FixtureDef.restitution = 1.0;
    FixtureDef.shape = &Shape;
    //Add to body
    body->CreateFixture(&FixtureDef);
    return body;
}

// Create a Box2d body with a box fixture, from a sfml::RectangleShape
b2Body* CreatePhysicsBox(b2World& world, const bool dynamic, const RectangleShape& rs) {
    return CreatePhysicsBox(world, dynamic, rs.getPosition(), rs.getSize());
}

void init() {
    const b2Vec2 gravity(0.0f, -10.0f);

    //Construct a world, which holds and simulates the physics bodies
    world = new b2World(gravity);

    //Wall Dimensions
    Vector2f walls[] = {
        //Top
        Vector2f(gameWidth * .5f, 5.f), Vector2f(gameWidth, 10.f),
        //Bottom
        Vector2f(gameWidth * .5f, gameHeight - 5.f), Vector2f(gameWidth, 10.f),
        //Left
        Vector2f(5.f, gameHeight * .5f), Vector2f(10.f, gameHeight),
        //Right
        Vector2f(gameWidth - 5.f, gameHeight * .5f), Vector2f(10.f, gameHeight),
    };

    //Setting box colours
    const Color box_cols[]{      {255,0,0},
                                 {0,255,0},
                                 {0,0,255},
                                 {175,0,255},
                                 {255,255,0}};


    //Build walls
    for (int i = 0; i < 7; i+=2) {
        //Create SFML shapes for each wall
        auto w = new RectangleShape();
        w->setPosition(walls[i]);
        w->setSize(walls[i + 1]);
        w->setOrigin(walls[i + 1] * .5f);
        w->setFillColor(Color::White);
        sprites.push_back(w);

        //Create static physics body for the wall
        auto p = CreatePhysicsBox(*world, false, *w);
        bodies.push_back(p);
    }
    //Create Boxes
    for (int i = 1; i < 11; ++i) {
        //Create SFML shapes for each box
        auto s = new RectangleShape();
        s->setPosition(Vector2f(i * (gameWidth / 12.f), gameHeight * .7f));
        s->setSize(Vector2f(50.0f, 50.0f));
        s->setOrigin(Vector2f(25.0f, 25.0f));
        s->setFillColor(box_cols[i % 5]);
        sprites.push_back(s);

        //Create a dynamic physics body for the box
        auto b = CreatePhysicsBox(*world, true, *s);
        //Give the box a spin
        b->ApplyAngularImpulse(5.0f, true);
        bodies.push_back(b);
    }
}

void Update() {
    static Clock clock;
    float dt = clock.restart().asSeconds();

    //Step Physics world by dt (non-fixed timestep) - THIS DOES ALL THE ACTUAL SIMULATION, DON'
    world -> Step(dt, velocityIterations, positionIterations);

    for (int i = 0; i < bodies.size(); ++i) {
        //Sync Sprites to physics position
        sprites[i]->setPosition(invert_height(bv2_to_sv2(bodies[i]->GetPosition())));
        //Sync Sprites to physics Rotation
        sprites[i]->setRotation((180 / b2_pi) * bodies[i]->GetAngle());
    }
}

void Render(RenderWindow &window) {
    for (auto s : sprites) {
        window.draw(*s);
    }
}

int main() {
    RenderWindow window(VideoMode(gameWidth, gameHeight), "Physics");
    init();

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
        }

        window.clear();
        Update();
        Render(window);
        window.display();
    }

    return 0;
}

