/*
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
 */

/** \file KX_RadarSensor.h
 *  \ingroup ketsji
 */

#ifndef __KX_RADARSENSOR_H__
#define __KX_RADARSENSOR_H__

#include "KX_NearSensor.h"

/**
 * Radar 'cone' sensor. Very similar to a near-sensor, but instead of a sphere, a cone is used.
 */
class KX_RadarSensor : public KX_NearSensor
{
 protected:
	Py_Header
		
	float		m_coneradius;

	/**
	 * Height of the cone.
	 */
	float		m_coneheight;
	int				m_axis;

	/**
	 * The previous position of the origin of the cone.
	 */
	mt::vec3 m_cone_origin;

	/**
	 * The previous direction of the cone (origin to bottom plane).
	 */
	mt::vec3 m_cone_target;
	
public:

	KX_RadarSensor(SCA_EventManager* eventmgr,
		KX_GameObject* gameobj,
			PHY_IPhysicsController* physCtrl,
			double coneradius,
			double coneheight,
			int	axis,
			double margin,
			double resetmargin,
			bool bFindMaterial,
			const std::string& touchedpropname);
	KX_RadarSensor();
	virtual ~KX_RadarSensor();
	virtual void SynchronizeTransform();
	virtual EXP_Value* GetReplica();

	/* --------------------------------------------------------------------- */
	/* Python interface ---------------------------------------------------- */
	/* --------------------------------------------------------------------- */
	enum RadarAxis {
		KX_RADAR_AXIS_POS_X = 0,
		KX_RADAR_AXIS_POS_Y,
		KX_RADAR_AXIS_POS_Z,
		KX_RADAR_AXIS_NEG_X,
		KX_RADAR_AXIS_NEG_Y,
		KX_RADAR_AXIS_NEG_Z
	};

	virtual sensortype GetSensorType() { return ST_RADAR; }
	/* python */
#ifdef WITH_PYTHON
	static PyObject*	pyattr_get_angle(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
#endif
};

#endif  /* __KX_RADARSENSOR_H__ */
