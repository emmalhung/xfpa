/***********************************************************************
*                                                                      *
*     u n i x . c                                                      *
*                                                                      *
*     Assorted unix system calls with practical embellishments.        *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                      *
***********************************************************************/

#include <string.h>
#include <stdio.h>
#include <fpa.h>

/**********************************************************************/

int	main(void)

	{
	STRING	name;
	UNLONG	id;
	int		i, nip, ipa, ipb, ipc, ipd;
	STRING	*iplist;

	name = fpa_host_name();
	printf("Host Name: %s\n", name);

	id = fpa_host_id();
	printf("Machine ID: %lu (0x%lx)\n", id, id);

	nip = fpa_host_ip_list(&iplist);
	for (i=0; i<nip; i++)
		{
		ipa = iplist[i][0];	if (ipa < 0) ipa += 256;
		ipb = iplist[i][1];	if (ipb < 0) ipb += 256;
		ipc = iplist[i][2];	if (ipc < 0) ipc += 256;
		ipd = iplist[i][3];	if (ipd < 0) ipd += 256;
		printf("IP Address: %d.%d.%d.%d\n", ipa, ipb, ipc, ipd);
		}
	}
