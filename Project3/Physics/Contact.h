#ifndef Contact_h_
#define Contact_h_

#include "Object.h"
#include "Physics/Fixture.h"
#include "Physics/Manifold.h"
#include "utils.h"

class Contact
{
public:
  Fixture fixtures[2];
  int vertexIndices[2];

  Manifold manifold;

  glm::f32 timeOfImpact; // 0 -> 1 fraction of current step;

  glm::f64 contactFriction;
  glm::f64 contactRestitution;
  glm::f64vec2 relativeVelocity;
  
  void applyImpulse()
  {
    Object * obj1 = fixtures[0].object;
    Object * obj2 = fixtures[1].object;
    for(unsigned short i = 0; i < manifold.pointCount; ++i)
    {
      // Calculate radii from COM to contact
      glm::f64vec2 radiiObj1 = (glm::f64vec2)(manifold.contactPoints[i].contactPoint - obj1->getPos());
      glm::f64vec2 radiiObj2 = (glm::f64vec2)(manifold.contactPoints[i].contactPoint - obj2->getPos());

      // Relative velocity
      glm::f64vec2 velocity = (glm::f64vec2)obj2->getSpeed() + mika::crossS(obj2->getRotSpeed(), radiiObj2 ) -
                              (glm::f64vec2)obj1->getSpeed() - mika::crossS(obj1->getRotSpeed(), radiiObj1 );

      // Relative velocity along the normal
      glm::f64 contactVel = glm::dot(velocity, manifold.localNormal);

      // Do not resolve if velocities are separating
      if(contactVel > 0
      || contactVel != contactVel
      )
        return;

      glm::f64 raCross1 = mika::cross(radiiObj1, manifold.localNormal);
      glm::f64 raCross2 = mika::cross(radiiObj2, manifold.localNormal);
      glm::f64 invMassSum = obj1->getInvMass() + obj2->getInvMass() + raCross1 * raCross1 * obj1->getInvInertia() + raCross2 * raCross2 * obj2->getInvInertia();

      // Calculate impulse scalar
      glm::f64 impulseScalar = -(1.0f + contactRestitution) * contactVel;
      impulseScalar /= invMassSum;
      impulseScalar /= (glm::f64)manifold.pointCount;

      // Apply impulse
      glm::f64vec2 impulse = manifold.localNormal * impulseScalar;
      obj1->applyImpulse( -impulse, radiiObj1 );
      obj2->applyImpulse( impulse, radiiObj2 );

      glm::f64vec2 t = velocity - (manifold.localNormal * glm::dot(velocity, manifold.localNormal));
      t = glm::normalize(t);

      // j tangent magnitude
      glm::f64 tangentScalar = -glm::dot(velocity, t);
      tangentScalar /= invMassSum;
      tangentScalar /= (glm::f64)manifold.pointCount;

      // Don't apply tiny friction impulses
      if(tangentScalar == 0)
      {
        return;
      }

      // Coulumb's law
      glm::f64vec2 tangentImpulse;
      if(std::abs( tangentScalar ) < impulseScalar * contactFriction)
        tangentImpulse = t * tangentScalar;
      else
        tangentImpulse = t * -impulseScalar * contactFriction;

      // Apply friction impulse
      obj1->applyImpulse( -tangentImpulse, radiiObj1 );
      obj2->applyImpulse(  tangentImpulse, radiiObj2 );
    }
  }
};

#endif