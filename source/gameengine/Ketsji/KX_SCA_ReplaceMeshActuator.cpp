/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file gameengine/Ketsji/KX_SCA_ReplaceMeshActuator.cpp
 *  \ingroup ketsji
 *
 * Replace the mesh for this actuator's parent
 */

//
// Previously existed as:

// \source\gameengine\GameLogic\SCA_ReplaceMeshActuator.cpp

// Please look here for revision history.

#include "KX_SCA_ReplaceMeshActuator.h"
#include "KX_Scene.h"
#include "KX_GameObject.h"
#include "KX_MeshProxy.h"

#ifdef WITH_PYTHON

/* ------------------------------------------------------------------------- */
/* Python functions                                                          */
/* ------------------------------------------------------------------------- */

/* Integration hooks ------------------------------------------------------- */

PyTypeObject KX_SCA_ReplaceMeshActuator::Type = {
    PyVarObject_HEAD_INIT(nullptr, 0) "KX_SCA_ReplaceMeshActuator",
    sizeof(PyObjectPlus_Proxy),
    0,
    py_base_dealloc,
    0,
    0,
    0,
    0,
    py_base_repr,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    Methods,
    0,
    0,
    &SCA_IActuator::Type,
    0,
    0,
    0,
    0,
    0,
    0,
    py_base_new};

PyMethodDef KX_SCA_ReplaceMeshActuator::Methods[] = {
    KX_PYMETHODTABLE(KX_SCA_ReplaceMeshActuator, instantReplaceMesh),
    {nullptr, nullptr}  // Sentinel
};

PyAttributeDef KX_SCA_ReplaceMeshActuator::Attributes[] = {
    KX_PYATTRIBUTE_RW_FUNCTION(
        "mesh", KX_SCA_ReplaceMeshActuator, pyattr_get_mesh, pyattr_set_mesh),
    KX_PYATTRIBUTE_BOOL_RW("useDisplayMesh", KX_SCA_ReplaceMeshActuator, m_use_gfx),
    KX_PYATTRIBUTE_BOOL_RW("usePhysicsMesh", KX_SCA_ReplaceMeshActuator, m_use_phys),
    KX_PYATTRIBUTE_NULL  // Sentinel
};

PyObject *KX_SCA_ReplaceMeshActuator::pyattr_get_mesh(PyObjectPlus *self,
                                                      const struct KX_PYATTRIBUTE_DEF *attrdef)
{
  KX_SCA_ReplaceMeshActuator *actuator = static_cast<KX_SCA_ReplaceMeshActuator *>(self);
  if (!actuator->m_mesh)
    Py_RETURN_NONE;
  KX_MeshProxy *meshproxy = new KX_MeshProxy(actuator->m_mesh);
  return meshproxy->NewProxy(true);
}

int KX_SCA_ReplaceMeshActuator::pyattr_set_mesh(PyObjectPlus *self,
                                                const struct KX_PYATTRIBUTE_DEF *attrdef,
                                                PyObject *value)
{
  KX_SCA_ReplaceMeshActuator *actuator = static_cast<KX_SCA_ReplaceMeshActuator *>(self);
  RAS_MeshObject *new_mesh;

  if (!ConvertPythonToMesh(actuator->GetLogicManager(),
                           value,
                           &new_mesh,
                           true,
                           "actuator.mesh = value: KX_SCA_ReplaceMeshActuator"))
    return PY_SET_ATTR_FAIL;

  actuator->m_mesh = new_mesh;
  return PY_SET_ATTR_SUCCESS;
}

KX_PYMETHODDEF_DOC(KX_SCA_ReplaceMeshActuator,
                   instantReplaceMesh,
                   "instantReplaceMesh() : immediately replace mesh without delay\n")
{
  InstantReplaceMesh();
  Py_RETURN_NONE;
}

#endif  // WITH_PYTHON

/* ------------------------------------------------------------------------- */
/* Native functions                                                          */
/* ------------------------------------------------------------------------- */

KX_SCA_ReplaceMeshActuator::KX_SCA_ReplaceMeshActuator(
    KX_GameObject *gameobj, RAS_MeshObject *mesh, KX_Scene *scene, bool use_gfx, bool use_phys)
    :

      SCA_IActuator(gameobj, KX_ACT_REPLACE_MESH),
      m_mesh(mesh),
      m_scene(scene),
      m_use_gfx(use_gfx),
      m_use_phys(use_phys)
{
} /* End of constructor */

KX_SCA_ReplaceMeshActuator::~KX_SCA_ReplaceMeshActuator()
{
  // there's nothing to be done here, really....
} /* end of destructor */

bool KX_SCA_ReplaceMeshActuator::Update()
{
  // bool result = false;	/*unused*/
  bool bNegativeEvent = IsNegativeEvent();
  RemoveAllEvents();

  if (bNegativeEvent)
    return false;  // do nothing on negative events

  if (m_mesh || m_use_phys) /* nullptr mesh is ok if were updating physics */
    m_scene->ReplaceMesh(static_cast<KX_GameObject *>(GetParent()), m_mesh, m_use_gfx, m_use_phys);

  return false;
}

CValue *KX_SCA_ReplaceMeshActuator::GetReplica()
{
  KX_SCA_ReplaceMeshActuator *replica = new KX_SCA_ReplaceMeshActuator(*this);

  if (replica == nullptr)
    return nullptr;

  replica->ProcessReplica();

  return replica;
};

void KX_SCA_ReplaceMeshActuator::InstantReplaceMesh()
{
  if (m_mesh)
    m_scene->ReplaceMesh(static_cast<KX_GameObject *>(GetParent()), m_mesh, m_use_gfx, m_use_phys);
}

void KX_SCA_ReplaceMeshActuator::Replace_IScene(SCA_IScene *val)
{
  m_scene = static_cast<KX_Scene *>(val);
}

/* eof */
