#!/usr/bin/python
# ------------------------------------------------------------
# File        : ltab2map
# Author      : Emma Hung
# Company     : Environment Canada
# Date        : 08/26/2008
# Description : 
# ------------------------------------------------------------
 
# ------------------------------------------------------------
# Setup Environment
# ------------------------------------------------------------
import sys

# ------------------------------------------------------------
# Do some work here
# ------------------------------------------------------------

#############################################################
# printHeader
def printHeader(FPAfield):
	print("* This file was created using ltab2map.py")
	print("******************************************")
	print("rev 2.0")
	print("")
	print(" projection latitude_longitude")
	print(" mapdef 0 0 0 -90 -180 90 180 1")
	print(" units latlon")
	print("")
	print("*")
	print("* %s" % FPAfield)
	print("field scattered %s geography" % FPAfield)
# end printHeader
#############################################################
 
#############################################################
# printFooter
def printFooter():
	print("*")
	print("* End")
# end printFooter
#############################################################
 
#############################################################
# printItem
def printItem(item, FPAcategory, FPAclass):
	if len(item) > 2:
		print('value %d' % (len(item) - len(item[4:]) ) )
		print('	FPA_category \"%s\"'    % (FPAcategory ) )
		print('	FPA_auto_label \"%s\"'  % (item[0] ) )
		if len(item) > 3 and item[3] != '-':
			print('	FPA_timestamp \"%s\"'   % (item[3] ) )
		else:
			print('	FPA_timestamp \"\"')
		if len(item) > 4:
			if " ".join(item[4:]).count('"') > 1:
				print('	FPA_user_label %s'  % (" ".join(item[4:]) ) )
			else:
				print('	FPA_user_label \"%s\"'  % (" ".join(item[4:]) ) )
		print('spot %s %s \"%s\" none' % (item[1], item[2], FPAclass) )
# end printItem
#############################################################

#############################################################
# Main Program
#############################################################
# Check command line arguments
if len(sys.argv) > 1:
	FPAfield = sys.argv[1]
else:
	print ("Missing FPA field argument.\nAborting")
	sys.exit()
if len(sys.argv) > 2:
	FPAclass = sys.argv[2]
else:
	FPAclass = "plot"
if len(sys.argv) > 3:
	FPAcategory = sys.argv[3]
else:
	FPAcategory = "none"
# Read from standard input
items = [ s.rstrip() for s in sys.stdin.readlines() ]
# Print Header
printHeader(FPAfield)
# skip comment lines
for item in items:
	if item.count('#'): item = item[:item.find('#')].rstrip() # remove comments
	if item.count('!'): item = item[:item.find('!')].rstrip() # remove comments
	printItem(item.split(), FPAcategory, FPAclass)

# Print Footer
printFooter()
# ------------------------------------------------------------
# Done
# ------------------------------------------------------------
