/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 2001 by Systems in Motion. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  version 2.1 as published by the Free Software Foundation. See the
 *  file LICENSE.LGPL at the root directory of the distribution for
 *  more details.
 *
 *  If you want to use Coin for applications not compatible with the
 *  LGPL, please contact SIM to acquire a Professional Edition license.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

/*!
  \class SoVRMLExtrusion SoVRMLExtrusion.h Inventor/VRMLnodes/SoVRMLExtrusion.h
  \brief The SoVRMLExtrusion class is a a geometry node for extruding a cross section along a spine.
*/

/*!
  SoSFBool SoVRMLExtrusion::beginCap
  Used to enable/disable begin cap. Default value is TRUE.
*/

/*!
  SoSFBool SoVRMLExtrusion::ccw
  Specifies counterclockwise vertex ordering. Default value is TRUE.
*/

/*!
  SoSFBool SoVRMLExtrusion::convex
  Specifies if cross sections is convex. Default value is TRUE.
*/

/*!
  SoSFFloat SoVRMLExtrusion::creaseAngle
  Specifies the crease angle for the generated normals. Default value is 0.0.
*/

/*!
  SoMFVec2f SoVRMLExtrusion::crossSection
  The cross section.
*/

/*!
  SoSFBool SoVRMLExtrusion::endCap
  Used to enable/disable end cap. Default value is TRUE.
  
*/

/*!
  SoMFRotation SoVRMLExtrusion::orientation
  Orientation for the cross section at each spine point.
*/

/*!
  SoMFVec2f SoVRMLExtrusion::scale
  Scaling for the cross section at each spine point.
*/

/*!
  SoSFBool SoVRMLExtrusion::solid
  When TRUE, backface culling will be enabled. Default value is TRUE.
*/

/*!
  SoMFVec3f SoVRMLExtrusion::spine
  The spine points.
*/


#include <Inventor/VRMLnodes/SoVRMLExtrusion.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/SbTesselator.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoGL.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/errors/SoDebugError.h>
#include <float.h>
#include <math.h>


//
// needed to avoid warnings generated by SbVec3f::normalize
//
static float
my_normalize(SbVec3f & vec)
{
  float len = vec.length();
  if (len > FLT_EPSILON) {
    vec /= len;
  }
  return len;
}


#ifndef DOXYGEN_SKIP_THIS
class SoVRMLExtrusionP {
public:
  SoVRMLExtrusionP(SoVRMLExtrusion * master)
    :master(master),
     coord(32),
     tcoord(32),
     idx(32),
     gen(TRUE),
     tess(tess_callback, this),
     dirty(TRUE)
  {
  }

  SoVRMLExtrusion * master;
  SbList <SbVec3f> coord;
  SbList <SbVec2f> tcoord;
  SbList <int> idx;
  SoNormalGenerator gen;
  SbTesselator tess;
  SbBool dirty;

  static void tess_callback(void *, void *, void *, void *);
  void generateCoords(void);
  void generateNormals(void);
};
#endif // DOXYGEN_SKIP_THIS

#undef THIS
#define THIS this->pimpl


SO_NODE_SOURCE(SoVRMLExtrusion);

// Doc in parent
void
SoVRMLExtrusion::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLExtrusion, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLExtrusion::SoVRMLExtrusion(void)
{
  THIS = new SoVRMLExtrusionP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoVRMLExtrusion);

  SO_VRMLNODE_ADD_FIELD(beginCap, (TRUE));
  SO_VRMLNODE_ADD_FIELD(endCap, (TRUE));
  SO_VRMLNODE_ADD_FIELD(solid, (TRUE));
  SO_VRMLNODE_ADD_FIELD(ccw, (TRUE));
  SO_VRMLNODE_ADD_FIELD(convex, (TRUE));
  SO_VRMLNODE_ADD_FIELD(creaseAngle, (0.0f));

  SO_NODE_ADD_FIELD(crossSection, (0.0f, 0.0f));
  this->crossSection.setNum(5);
  SbVec2f * cs = this->crossSection.startEditing();
  cs[0] = SbVec2f(1.0f, 1.0f);
  cs[1] = SbVec2f(1.0f, -1.0f);
  cs[2] = SbVec2f(-1.0f, -1.0f);
  cs[3] = SbVec2f(-1.0f, 1.0f);
  cs[4] = SbVec2f(1.0f, 1.0f);
  this->crossSection.finishEditing();
  this->crossSection.setDefault(TRUE);

  SO_NODE_ADD_FIELD(orientation, (SbRotation::identity()));
  SO_NODE_ADD_FIELD(scale, (1.0f, 1.0f));

  SO_NODE_ADD_FIELD(spine, (0.0f, 0.0f, 0.0f));
  this->spine.setNum(2);
  this->spine.set1Value(1, 0.0f, 1.0f, 0.0f);
  this->spine.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoVRMLExtrusion::~SoVRMLExtrusion()
{
  delete THIS;
}


// Doc in parent
void
SoVRMLExtrusion::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  this->updateCache();

  SoState * state = action->getState();

  SbBool doTextures = SoGLTextureEnabledElement::get(state);
  const SbVec3f * normals = THIS->gen.getNormals();

  SoCoordinateElement::set3(state, this, THIS->coord.getLength(), THIS->coord.getArrayPtr());
  const SoCoordinateElement * coords = SoCoordinateElement::getInstance(state);

  if (doTextures) {
    SoTextureCoordinateElement::set2(state, this, THIS->tcoord.getLength(),
                                     THIS->tcoord.getArrayPtr());
  }

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  this->setupShapeHints(state, this->ccw.getValue(), this->solid.getValue());

  sogl_render_faceset((SoGLCoordinateElement *) coords,
                      THIS->idx.getArrayPtr(),
                      THIS->idx.getLength(),
                      normals,
                      NULL,
                      &mb,
                      NULL,
                      NULL, /* &tb */
                      THIS->idx.getArrayPtr(),
                      3, /* SoIndexedFaceSet::PER_VERTEX */
                      0,
                      doTextures?1:0);
}

// Doc in parent
void
SoVRMLExtrusion::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  this->updateCache();
}

// Doc in parent
void
SoVRMLExtrusion::computeBBox(SoAction * action,
                             SbBox3f & box,
                             SbVec3f & center)
{
  this->updateCache();

  int num = THIS->coord.getLength();
  const SbVec3f * coords = THIS->coord.getArrayPtr();

  box.makeEmpty();
  while (num--) {
    box.extendBy(*coords++);
  }
  if (!box.isEmpty()) center = box.getCenter();

}

// Doc in parent
void
SoVRMLExtrusion::generatePrimitives(SoAction * action)
{
  this->updateCache();

  const SbVec3f * normals = THIS->gen.getNormals();
  const SbVec2f * tcoords = THIS->tcoord.getArrayPtr();
  const SbVec3f * coords = THIS->coord.getArrayPtr();
  const int32_t * iptr = THIS->idx.getArrayPtr();
  const int32_t * endptr = iptr + THIS->idx.getLength();

  SoPrimitiveVertex vertex;

  int idx;
  this->beginShape(action, TRIANGLES);
  while (iptr < endptr) {
    idx = *iptr++;
    while (idx >= 0) {
      vertex.setNormal(*normals++);
      vertex.setTextureCoords(tcoords[idx]);
      vertex.setPoint(coords[idx]);
      this->shapeVertex(&vertex);
      idx = *iptr++;
    }
  }
  this->endShape();
}

//
// private method that updates the coordinate and normal cache.
//
void
SoVRMLExtrusion::updateCache(void)
{
  if (THIS->dirty) {
    THIS->generateCoords();
    THIS->generateNormals();
    THIS->dirty = FALSE;
  }
}

// Doc in parent
void
SoVRMLExtrusion::notify(SoNotList * list)
{
  THIS->dirty = TRUE;
  inherited::notify(list);
}


// Doc in parent
SoDetail * 
SoVRMLExtrusion::createTriangleDetail(SoRayPickAction * action,
                                      const SoPrimitiveVertex * v1,
                                      const SoPrimitiveVertex * v2,
                                      const SoPrimitiveVertex * v3,
                                      SoPickedPoint * pp)
{
  // no triangle detail for Extrusion
  return NULL;
}

#undef THIS
#ifndef DOXYGEN_SKIP_THIS

//
// generates extruded coordinates
//
void
SoVRMLExtrusionP::generateCoords(void)
{
  this->coord.truncate(0);
  this->tcoord.truncate(0);
  this->idx.truncate(0);

  if (this->master->crossSection.getNum() == 0 ||
      this->master->spine.getNum() == 0) return;

  SbMatrix matrix = SbMatrix::identity();

  int i, j, numcross;
  SbBool connected = FALSE;   // is cross section closed
  SbBool closed = FALSE;      // is spine closed
  numcross = this->master->crossSection.getNum();
  const SbVec2f * cross =  master->crossSection.getValues(0);
  if (cross[0] == cross[numcross-1]) {
    connected = TRUE;
    numcross--;
  }

  int numspine = master->spine.getNum();
  const SbVec3f * spine = master->spine.getValues(0);
  if (spine[0] == spine[numspine-1]) {
    closed = TRUE;
    numspine--;
  }

  SbVec3f X, Y, Z;
  SbVec3f prevX(1.0f, 0.0f, 0.0f);
  SbVec3f prevY(0.0f, 1.0f, 0.0f);
  SbVec3f prevZ(0.0f, 0.0f, 1.0f);

  int numorient = this->master->orientation.getNum();
  const SbRotation * orient = this->master->orientation.getValues(0);

  int numscale = this->master->scale.getNum();
  const SbVec2f * scale = this->master->scale.getValues(0);

  int reversecnt = 0;

  // loop through all spines
  for (i = 0; i < numspine; i++) {
    if (closed) {
      if (i > 0)
        Y = spine[i+1] - spine[i-1];
      else
        Y = spine[1] - spine[numspine-1];
    }
    else {
      if (i == 0) Y = spine[1] - spine[0];
      else if (i == numspine-1) Y = spine[numspine-1] - spine[numspine-2];
      else Y = spine[i+1] - spine[i-1];
    }

    if (my_normalize(Y) <= FLT_EPSILON) {
      if (prevY[1] < 0.0f)
        Y = SbVec3f(0.0f, -1.0f, 0.0f);
      else
        Y = SbVec3f(0.0f, 1.0f, 0.0f);
    }

    SbVec3f z0, z1;

    if (closed) {
      if (i > 0) {
        z0 = spine[i+1] - spine[i];
        z1 = spine[i-1] - spine[i];
      }
      else {
        z0 = spine[1] - spine[0];
        z1 = spine[numspine-1] - spine[0];
      }
    }
    else {
      if (numspine == 2) {
        z0 = SbVec3f(1.0f, 0.0f, 0.0f);
        z1 = Y;
        Z = z0.cross(z1);
        if (my_normalize(Z) <= FLT_EPSILON) {
          z0 = SbVec3f(0.0f, 1.0f, 0.0f);
          z1 = Y;
        }
      }
      else if (i == 0) {
        z0 = spine[2] - spine[1];
        z1 = spine[0] - spine[1];
      }
      else if (i == numspine-1) {
        z0 = spine[numspine-1] - spine[numspine-2];
        z1 = spine[numspine-3] - spine[numspine-2];
      }
      else {
        z0 = spine[i+1] - spine[i];
        z1 = spine[i-1] - spine[i];
      }
    }
    
    my_normalize(z0);
    my_normalize(z1);

    // test if spine segments are parallel. If they are, the cross
    // product will not be reliable, and we should just use the
    // previous Z-axis instead.
    if (z0.dot(z1) < -0.999f) {
      Z = prevZ;
    }
    else {
      Z = z0.cross(z1);
    }

    if (my_normalize(Z) <= FLT_EPSILON || SbAbs(Y.dot(Z)) > 0.5f) {
      Z = SbVec3f(0.0f, 0.0f, 0.0f);
      
      int bigy = 0;
      float bigyval = Y[0];
      if (SbAbs(Y[1]) > SbAbs(bigyval)) {
        bigyval = Y[1];
        bigy = 1;
      }
      if (SbAbs(Y[2]) > SbAbs(bigyval)) {
        bigy = 2;
        bigyval = Y[2];
      }
      Z[(bigy+1)%3] = bigyval > 0.0f ? 1.0f : -1.0f;

      // make Z perpendicular to Y
      X = Y.cross(Z);
      my_normalize(X);
      Z = X.cross(Y);
      my_normalize(Z);
    }

    X = Y.cross(Z);
    my_normalize(X);

#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoVRMLExtrusionP::generateCoords",
                           "\nPre Zdot: %g, Xdot: %g",
                           Z.dot(prevZ), X.dot(prevX));
#endif // debug

    if (i > 0 && (Z.dot(prevZ) <= 0.5f || X.dot(prevX) <= 0.5f)) {
      // if change is fairly large, try to find the most appropriate
      // axis. This will minimize change from spine-point to
      // spine-point
      SbVec3f v[4];
      v[0] = X;
      v[1] = -X;
      v[2] = Z;
      v[3] = -Z;
      
      float maxdot = v[0].dot(prevZ);
      int maxcnt = 0;
      for (int cnt = 1; cnt < 4; cnt++) {
        float dot = v[cnt].dot(prevZ);
        if (dot > maxdot) {
          maxdot = dot;
          maxcnt = cnt;
        }
      }
      Z = v[maxcnt];
      X = Y.cross(Z);
      my_normalize(X);
    }

#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoVRMLExtrusionP::generateCoords",
                           "\nPost Zdot: %g, Xdot: %g",
                           Z.dot(prevZ), X.dot(prevX));
#endif // debug

#if COIN_DEBUG && 0
    SoDebugError::post("ext",
                       "\nX: %g %g %g\n"
                       "Y: %g %g %g\n"
                       "Z: %g %g %g\n\n",
                       X[0], X[1], X[2],
                       Y[0], Y[1], Y[2],
                       Z[0], Z[1], Z[2]);
#endif // debug

#if 0 // testing zAxis locking
    Z = SbVec3f(0.0f, 0.0f, 1.0f);
    X = Y.cross(Z);
    X.normalize();
    Z = X.cross(Y);
    Z.normalize();
#endif // debug

    prevX = X;
    prevY = Y;
    prevZ = Z;

    matrix[0][0] = X[0];
    matrix[0][1] = X[1];
    matrix[0][2] = X[2];
    matrix[0][3] = 0.0f;

    matrix[1][0] = Y[0];
    matrix[1][1] = Y[1];
    matrix[1][2] = Y[2];
    matrix[1][3] = 0.0f;

    matrix[2][0] = Z[0];
    matrix[2][1] = Z[1];
    matrix[2][2] = Z[2];
    matrix[2][3] = 0.0f;

    matrix[3][0] = spine[i][0];
    matrix[3][1] = spine[i][1];
    matrix[3][2] = spine[i][2];
    matrix[3][3] = 1.0f;

    int cnt = 0;
    if (X[0] < 0.0f) cnt++;
    if (X[1] < 0.0f) cnt++;
    if (X[2] < 0.0f) cnt++;
    if (Y[0] < 0.0f) cnt++;
    if (Y[1] < 0.0f) cnt++;
    if (Y[2] < 0.0f) cnt++;
    if (Z[0] < 0.0f) cnt++;
    if (Z[1] < 0.0f) cnt++;
    if (Z[2] < 0.0f) cnt++;

    if (cnt & 1) reversecnt--;
    else reversecnt++;

    if (numorient) {
      SbMatrix rmat;
      orient[SbMin(i, numorient-1)].getValue(rmat);
      matrix.multLeft(rmat);
    }

    if (numscale) {
      SbMatrix smat = SbMatrix::identity();
      SbVec2f s = scale[SbMin(i, numscale-1)];
      smat[0][0] = s[0];
      smat[2][2] = s[1];
      matrix.multLeft(smat);
    }

    for (j = 0; j < numcross; j++) {
      SbVec3f c;
      c[0] = cross[j][0];
      c[1] = 0.0f;
      c[2] = cross[j][1];

      matrix.multVecMatrix(c, c);
      this->coord.append(c);
      this->tcoord.append(SbVec2f(float(j)/float(connected ? numcross : numcross-1),
                                  float(i)/float(closed ? numspine : numspine-1)));
    }
  }

  // this macro makes the code below more readable
#define ADD_TRIANGLE(i0, j0, i1, j1, i2, j2) \
  do { \
    if (reversecnt < 0) { \
      this->idx.append((i2)*numcross+(j2)); \
      this->idx.append((i1)*numcross+(j1)); \
      this->idx.append((i0)*numcross+(j0)); \
      this->idx.append(-1); \
    } \
    else { \
      this->idx.append((i0)*numcross+(j0)); \
      this->idx.append((i1)*numcross+(j1)); \
      this->idx.append((i2)*numcross+(j2)); \
      this->idx.append(-1); \
    } \
  } while (0)
  
  // create beginCap polygon
  if (this->master->beginCap.getValue() && !closed) {
    if (this->master->convex.getValue()) {
      for (i = 1; i < numcross-1; i++) {
        ADD_TRIANGLE(0, 0, 0, i, 0, i+1);
      }
    }
    else {
      // let the tesselator create triangles
      this->tess.beginPolygon();
      for (i = 0; i < numcross; i++) {
        this->tess.addVertex(this->coord[i], (void*) i);
      }
      this->tess.endPolygon();
    }
  }


  // create endCap polygon
  if (this->master->endCap.getValue() && !closed) {
    if (this->master->convex.getValue()) {
      for (i = 1; i < numcross-1; i++) {
        ADD_TRIANGLE(numspine-1, numcross-1, 
                     numspine-1, numcross-1-i, 
                     numspine-1, numcross-2-i);
      }
    }
    else {
      // let the tesselator create triangles
      this->tess.beginPolygon();
      for (i = 0; i < numcross; i++) {
        int idx = (numspine-1)*numcross + numcross - 1 - i;
        this->tess.addVertex(this->coord[idx], (void*) idx);
      }
      this->tess.endPolygon();
    }
  }

  // create walls
  for (i = 0; i < numspine-1; i++) {
    for (j = 0; j < numcross-1; j++) {
      ADD_TRIANGLE(i, j, i+1, j, i+1, j+1);
      ADD_TRIANGLE(i, j, i+1, j+1, i, j+1);
    }
    if (connected) {
      ADD_TRIANGLE(i, j, i+1, j, i+1, 0);
      ADD_TRIANGLE(i, j, i+1, 0, i, 0);
    }
  }
  if (closed) {
    for (j = 0; j < numcross-1; j++) {
      ADD_TRIANGLE(numspine-1, j, 0, j, 0, j+1);
      ADD_TRIANGLE(numspine-1, j, 0, j+1, numspine-1, j+1);
    }
    if (connected) {
      ADD_TRIANGLE(numspine-1, j, 0, j, 0, 0);
      ADD_TRIANGLE(numspine-1, j, 0, 0, numspine-1, 0);
    }
  }
#undef ADD_TRIANGLE
}

//
// generates per-verex normals for the extrusion.
//
void
SoVRMLExtrusionP::generateNormals(void)
{
  this->gen.reset(this->master->ccw.getValue());
  const SbVec3f * c = this->coord.getArrayPtr();
  const int * iptr = this->idx.getArrayPtr();
  const int * endptr = iptr + this->idx.getLength();

  while (iptr < endptr) {
    this->gen.beginPolygon();
    int idx = *iptr++;
    while (idx >= 0) {
      this->gen.polygonVertex(c[idx]);
      idx = *iptr++;
    }
    this->gen.endPolygon();
  }
  this->gen.generate(this->master->creaseAngle.getValue());
}

//
// callback from the polygon tessellator
//
void
SoVRMLExtrusionP::tess_callback(void * v0, void * v1, void * v2, void * data)
{
  SoVRMLExtrusionP * thisp = (SoVRMLExtrusionP*) data;
  thisp->idx.append((int) v0);
  thisp->idx.append((int) v1);
  thisp->idx.append((int) v2);
  thisp->idx.append(-1);
}

#endif // DOXYGEN_SKIP_THIS
