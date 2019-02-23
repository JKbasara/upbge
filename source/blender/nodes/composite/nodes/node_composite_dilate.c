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
 * The Original Code is Copyright (C) 2006 Blender Foundation.
 * All rights reserved.
 */

/** \file blender/nodes/composite/nodes/node_composite_dilate.c
 *  \ingroup cmpnodes
 */


#include "node_composite_util.h"


/* **************** Dilate/Erode ******************** */

static bNodeSocketTemplate cmp_node_dilateerode_in[] = {
	{   SOCK_FLOAT, 1, N_("Mask"),      0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, PROP_NONE},
	{   -1, 0, ""   }
};
static bNodeSocketTemplate cmp_node_dilateerode_out[] = {
	{   SOCK_FLOAT, 0, N_("Mask")},
	{   -1, 0, ""   }
};

static void node_composit_init_dilateerode(bNodeTree *UNUSED(ntree), bNode *node)
{
	NodeDilateErode *data = MEM_callocN(sizeof(NodeDilateErode), "NodeDilateErode");
	data->falloff = PROP_SMOOTH;
	node->storage = data;
}

void register_node_type_cmp_dilateerode(void)
{
	static bNodeType ntype;

	cmp_node_type_base(&ntype, CMP_NODE_DILATEERODE, "Dilate/Erode", NODE_CLASS_OP_FILTER, 0);
	node_type_socket_templates(&ntype, cmp_node_dilateerode_in, cmp_node_dilateerode_out);
	node_type_init(&ntype, node_composit_init_dilateerode);
	node_type_storage(&ntype, "NodeDilateErode", node_free_standard_storage, node_copy_standard_storage);

	nodeRegisterType(&ntype);
}
