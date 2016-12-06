"FPA PYTHON RULE LIB"
import os
import sys

# Some common attribute keys:
FPA_field_labels          = "FPA_field_labels"
FPA_link_nodes            = "FPA_link_nodes"
FPA_latitude              = "FPA_latitude"
FPA_longitude             = "FPA_longitude"
FPA_proximity             = "FPA_proximity"
FPA_line_direction        = "FPA_line_direction"
FPA_line_length           = "FPA_line_length"
FPA_category              = "FPA_category"
FPA_auto_label            = "FPA_auto_label"
FPA_user_label            = "FPA_user_label"
FPA_wind_model            = "FPA_wind_model"
FPA_wind_direction        = "FPA_wind_direction"
FPA_wind_speed            = "FPA_wind_speed"
FPA_wind_gust             = "FPA_wind_gust"
FPA_line_type             = "FPA_line_type"
FPA_scattered_type        = "FPA_scattered_type"
FPA_lchain_reference      = "FPA_lchain_reference"
FPA_lchain_start_time     = "FPA_lchain_start_time"
FPA_lchain_end_time       = "FPA_lchain_end_time"
FPA_lnode_type            = "FPA_lnode_type"
FPA_lnode_time            = "FPA_lnode_time"
FPA_lnode_direction       = "FPA_lnode_direction"
FPA_lnode_speed           = "FPA_lnode_speed"
FPA_label_type            = "FPA_label_type"
FPA_label_feature         = "FPA_label_feature"
EVAL_contour			  = "EVAL_contour"
EVAL_spval  			  = "EVAL_spval"
EVAL_wind   			  = "EVAL_wind"
FPA_node_class_unknown    = "unknown"
Fpa_node_class_interp	  = "interp"
Fpa_node_class_control    = "control"
Fpa_node_class_normal     = "normal"

SCRIPT = os.path.basename(sys.argv[0])
INVALID_ATTRIBUTE = "[" + SCRIPT +"] ERROR: Invalid attribute: '%s'." 
NO_ATTRIBUTE      = "[" + SCRIPT +"] ERROR: No attribute: '%s'."
# Begin function print_dict(f,d)
def print_dict(fileName, dict):
	"Print dictionary to file."
	try:
		returnFileName = fileName[:16] + "list" + fileName[20:]
		dictFile = open(returnFileName,"w")
		dictFile.write( '%d\n' % len(dict))
	except IOError:
		print "[FPALib.py] Unable to open and write to file " + returnFileName
		return

	while ( dict ):
		(key, value) = dict.popitem()
		dictFile.write('"' + key + '" "' + value + '"\n')

	dictFile.close()
# End function print_dict(f,d)
	
# Begin function blank(s)
def blank(s): 
	"Return TRUE if s is empty or blank (white space only)."
	if len(s) == 0: return 1
	return s.isspace()
# End function blank(s)

# Begin function match(s1,s2)
def match(s1,s2):
	"Return TRUE if s2 is a substring of s1."
	if s1.count(s2) > 0: return 1
	else: return 0
# End function match(s1,s2)

# Begin function get_dict(f)
def get_dict(fileName):
	"Read file contents and convert them into a dictionary"
	try:
		dictFile = open(fileName,"r")
		line     = dictFile.readline()
	except IOError:
		print "[FPALib.py] Unable to open and read file " + fileName
		dict = {}
		return dict

	dictFile.close()

	try:
		dict = eval(line)
	except SyntaxError:
		print "[FPALib.py] Malformed dictionary in file " + fileName
		dict = {}
	
	return dict
# End function get_dict(f)

# Begin function build_wind_value_string
class WIND_VAL:
	dir   = 0.0
	dunit = "\260"
	speed = 0.0
	sunit = "knots"
	gust  = 0.0

	def build_wind_value_string(self):
		if self.dir < 0:	  return ""
		if self.speed < 0: return ""
		vdir  = round(self.dir,0)
		vspd  = round(self.speed,0)
		vgust = round(self.gust,0)
		if (vgust > vspd):
			return "%d%s %d:%d %s" % ( vdir, self.dunit, vspd, vgust, self.sunit)
		else:
			return "%d%s %d %s" % ( vdir, self.dunit, vspd, self.sunit)
# End function build_wind_value_string
