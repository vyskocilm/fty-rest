/*  =========================================================================
    fty_asset_uptime_configurator - Configuration for kpi-uptime

    Copyright (C) 2014 - 2017 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

#ifndef FTY_ASSET_UPTIME_CONFIGURATOR_H_INCLUDED
#define FTY_ASSET_UPTIME_CONFIGURATOR_H_INCLUDED

// inserts additional data to aux part of datacenter asset message
// *aux ... aux part of fty-proto message where info about DC and its UPSes is kept
// asset_name ... name of UPS.
//            UPS -> info about DC where it is located
//                -> list of UPSes located in the DC 
bool
   insert_upses_to_aux (zhash_t *aux, std::string asset_name);


#endif
