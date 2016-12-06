/*=========================================================================*/
/*
*	File:		dataEntryDialog_attributes.c
*
*	Purpose:	Allows entry of field attributes into the appropriate
*               data structure. The arrangement of the value entry objects
*               is read from a menu definition file.
*
*  Notes: This file contains the functions
*
*           ACTIVATE_areaAttributesDialog()
*           ACTIVATE_areaBkgndAttributesDialog()
*           ACTIVATE_labelAttributesDialog()
*           ACTIVATE_lineAttributesDialog()
*           ACTIVATE_linkChainAttributesDialog()
*           ACTIVATE_linkNodeAttributesDialog()
*           ACTIVATE_pointAttributesDialog()
*
*           MenuFileExists()
*           UpdateAttributesEntryDialog()
*           DestroyAttributesEntryDialog()
*
*
* -------- Attribute Entry Menu Definition File Description ---------------
*
*
* This file defines the contents of a specific entry menu configuration.
* There can be different files for different elements. Which entry file
* is associated with a specific element is set in the configuration
* files by the use of the "entry_file" keyword.
*
* NOTE: The function which parses this file is rather stupid so be careful.
*
* The first few lines define values which apply to the preset controls
* in the menu. These are (with example values):
*
*       menu_title              = "Synoptic Weather Entry Menu"
*       geometry                = -600 -300 600 600
*       label_display_height    = 1                  (default 1)
*       value_display_height    = 3                  (default 2)
*       category_display_width  = 10                 (default 20)
*       initialization          = normal             (default "normal")
*
*    The first entry, menu_title, sets the title to appear in the menu
*    frame at the very top of the window.
*
*    The second, geometry, sets the size of the entire entry menu as x y
*    width height all given in pixels. The position x and y is relative to
*    the button which causes the menu to appear and can thus have a negative
*    value. 
*
*    The third and fourth, label_display_height and value_display_height,
*    set the height, in rows, of the User Label and Auto Label displays
*    respectively. If the label_display_height and/or the value_display_height
*    are set to zero (0), then the given display will not appear in the menu.
*
*    The fifth entry, category_display_width, sets the width of the category
*    display which is just to the right of the Auto Label display. If the width
*    is set to zero (0), then the display will not appear in the menu.
*
*    If all three display sizes are set to zero, then none of them will appear.
*
*    The last entry, initialization, is optional and if present can have two
*    values, "none" or "normal". If none, the menu will always appear with the
*    menu items set to whatever this menu file defines as the startup state. If
*    normal, the menu will be filled in with the values of the last object that
*    had its attributes set or modified by the entry menu.
*
*
* After this all lines refer to dynamically configured and created
* entry objects.
*
* The entry objects are arranged into panels, with each panel controlled
* by a tab. The keyword is "panel" followed by the tab label enclosed
* in quotes.
*
* Within each panel can be arranged any number of objects. At the moment
* there are four types of objects defined: layout objects, attribute
* entry objects, control objects and display only objects.
*
* The layout objects are:
*
*   1. frame
*
* The attribute entry objects are:
*
*   1. popup_list
*   2. scrolled_list
*   3. button_list
*   4. spin_box
*   5. composite_list
*   6. text
*   7. radio_buttons
*
* The control object is:
*
*   1. run_program
*   2. clear_btn
*
* The display only objects are:
*
*   1. display_label
*   2. display_attribute
*   3. line
*
* ----------------------------------------------------------------------------
*
* Description of the objects available for use in the entry menu.
*
*  Object          Description
*  ------          -----------
*
*  button_list     This is a button, that when activated, pops up a list of
*                  choices. Unlike the popup_list below, there is no visual
*                  indication that the button will popup a list when activated.
*                  Note that this object is not available for old systems
*                  like HPUX 10x.
*
*  clear_btn       A button, which when pressed, clears a specified list
*                  of attribute objects.
*
*  composite_list  This object contains other objects that provide input
*                  to a single attribute. Any object can be a child of a
*                  composite, but only the button_list, popup_list,
*                  scrolled_list and spin_list objects will allow input.
*
*  display_label   Displays a given label in the menu. Used to provide static
*                  information to users.
*
*  display_attribute  Used to display the value of a given attribute with no
*                     modification capability.
*
*  frame           This is an object that encloses other object(s). It is
*                  used to "fancy up" the display. The frame can be given
*                  an optional label which is embedded in the upper left part
*                  of the frame border.
*
*  line            Displays either a horizontal or vertical line of various
*                  types.
*
*  popup_list      An object, indicated by a dash to the right of the display
*                  area, which pops up a list of choices when the mouse button
*                  is pushed down. When the mouse button is released the list
*                  pops down.
*
*  scrolled_list   An object, with a display area and an arrow displayed to
*                  the right of the area which pops up a list of choices. The
*                  list pops down when the mouse button is pushed outside of
*                  the list and selects an item if the mouse button is pushed
*                  on a list item.
*
*  spin_box        An object that displays a numeric value which is controlled
*                  by two arrows. The object must contain a value and cannot
*                  contain the idea of none, the object must have a value.
*
*  spin_list       An object that displays a value from a defined list of
*                  values in a spin_box style object. This can contain the
*                  idea of none.
*  radio_buttons   A list of buttons where only can be selected at a time.
*                  Selecting one deselects any other selected button.
*
*  run_program     A button that when activated will run a specified program.
*
*  text            An object that allows for the free form input of text.
*
* ----------------------------------------------------------------------------
*
* The list objects are for entering the value of any attribute from
* predefined list of values. The composite list does the same but
* constructs the attribute from several lists the contents of which
* are concatenated together. (For example the string RW-F could be
* made up of components from three lists, one of which contains
* RW, the second the - and the third the F.
*
* These objects are nested and related in the following manner.
*
*      panel "panel label"
*      {
*          entry object
*          {
*              keyword = value
*              ...
*          }
*          frame object
*          {
*              keyword = value
*              ...
*              entry object
*              {
*                  keyword = value
*                  ...
*              }
*              composite object
*              {
*                  keyword = value
*                  ...
*                  entry object
*                  {
*                      keyword = value
*                      ...
*                  }
*                  entry object
*                  {
*                      keyword = value
*                      ...
*                  }
*              }
*          }
*          entry object
*          {
*              keyword = value
*              ...
*          }
*      }
*      panel "panel label"
*      {
*          ...
*      }
*
* The characteristics of each of these objects is set by a list of
* keyword-value pairs. If part of a value string has embedded spaces
* this can be preserved by enclosing the part in quotes. For example:
*
*  label    = "Cloud Amount"
*  position = 241 377
*  size     = 500 370
*
*
* ------- Object Specification ---------------------------------------------
*
* Notes common to most objects.
*
* 1. Most objects have a number of columns setting. If not specified the
*    program will try to set the correct number, but due to font differences
*    you might have to set the number of columns "ncolumns" to more than
*    you might expect.
*
* 2. In all references to colour below, the colour is entered in normal X
*    notation. Some choices are:
*
*  - a colour name such as "wheat"
*  - by the rgb specification RGB:<red>/<green>/<blue> notation where
*    <red>,<green> and <blue> are each between 1 and 4 hexadecmial digits.
*  - by RGBi:<red>/<green>/<blue> where <red>,<green> and <blue> are
*    floating point numbers between 0.0 and 1.0 inclusive.
*
*    For a complete list of options see any X Windows reference book.
*
* 3. For those objects which take a list of items the following is valid:
*
*   The list of items to appear in the list must be separated by white space.
*   If any item must have embedded spaces, then the item must be enclosed in
*   quotes.
*
*   There is a special item with a name of "none" which can be inserted into
*   the list and when selected will set the current value of the corresponding
*   attribute to no value.
*
*   There is another special item that can be in the list, "range:". The syntax
*   for this is:
*                                
*       range:minimum,maximum,increment,decimals
*                                
*   Where minimum and maximum are the limits to a range of numbers, increment
*   is the amount to step between values by, and decimals is the number of
*   decimal points to display. If decimals is not specified or is 0 (zero)
*   then the range is taken to be a range of integers. If it is specified then
*   the range is taken to be a range of float values. It is important that 
*   there be no spaces beween the elements or that the entire range
*   specification be enclosed in quotes. There can be more than one range
*   specification in any item list.
*
* 4. For those objects which can have their list of values limited by another
*    object the following applies for the limit keyword:
*   
*   There are two parameters: the limit definition and the id (attribute_id or
*   composite_id) of the object doing the limiting. The limit definition must
*   be one of LT, LE, GE, GT. For example:
*
*      limit = GT cloud_base_lower
*
*   Some very simple logic can also be done limited to AND or OR. For example if
*   there were two objects defining the cloud base as a range, say with
*   composite_id's of cb1 and cb2, then we could have:
*
*     limit = GT base:cb1 AND GT base:cb2
*
*   How the values in the two objects are compared depends on the limit_type
*   key. The possible values for this are string, integer or float. This
*   determines if the comparison is done by comparing strings, integer values
*   or float values. The default is integer.
*
*   Note that if the numeric string contains fractional values like "1/4" or
*   "3/4", limit_type should be specified as float.
*
*   For numerical comparisons there are several assumptions:
*
*   1. The values go from smaller to larger in the string.
*   2. If an entry starts with a ">" symbol it is unbounded, and if it starts
*      with a "<" symbol it is very small.
*
*   See the composite_list object for information on the composite_id keyword.
*
*
*
* Available object types:
*
*   object        keyword        value
*   ------        -------        -----
*
* - Layout Objects -
*
*  frame          label          The label to display on the frame which is to
*                                enclose the other objects. If not given then
*                                the frame will be created without a label.
*
*                 position       The offset, as x and y in pixels, of the frame
*                                from the upper left corner of its parent which
*                                can be either a panel or another frame.
*
*                 size           The width and height, in pixels, of the frame.
*
*
* - Attribute Entry Objects -
*
*  button_list    attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 foreground     The foreground colour of the label.
*                                Default parent's.
*
*                 background     The background colour of the label.
*                                Default parent's.
*
*                 border_width   The width of the border around the label.
*                                Default 0.
*
*                 border_color   The colour of the border.
*
*                 margin_height  The height of the margin, in pixels, above and
*                                below the string in the label. Default 2.
*
*                 margin_width   The width of the margin, in pixels, to the
*                                right and left of the string in the label.
*                                Default 2.
*
*                 items          The list of attribute values to appear in the
*                                list, separated by white space. If any item must
*                                have embedded spaces, then it must be enclosed
*                                in quotes. There is a special item with a name
*                                of "none" which can be inserted into the list
*                                and when selected will set the current value
*                                of the corresponding attribute to no value.
*
*                 item_labels    Optional. If the names of the attribute values to
*                                be shown to the user needs to be different from
*                                the actual values as stored in the items parameter
*                                above, this is where this can be done. The number
*                                of item_labels needs to be the same as the number
*                                of items.
*
*                 none_sub       If one of the items is "none" this is the string
*                                to substitute. The user will see this substituted
*                                string but its action will be the same as "none".
*                                Only valid if item_labels is not defined.
*
*                 nvisible       The number of items which will be visible in
*                                the scrolled list area. Default is the number
*                                of items.
*
*                 ncolumns       The size of the display area specified in
*                                character columns. If not specified the
*                                default size is taken from the maximum length
*                                of the entries in the items list.
*
*                 default        The item from the above list to be used as the
*                                default value.
*
*                 limit          If the allowed values in the list are limited
*                                by some other entry object, this key defines the
*                                relationship. See note above.
*
*                 limit_type     One of "integer", "float" or "string". This has
*                                meaning only if a limit is specified and determines
*                                if the limit testing is numeric or by string
*                                comparison. Default is integer.
*
* 
*  popup_list     attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 items          The list of attribute values to appear in the
*                                list, separated by white space. If any item must
*                                have embedded spaces, then it must be enclosed
*                                in quotes. There is a special item with a name
*                                of "none" which can be inserted into the list
*                                and when selected will set the current value of
*                                the corresponding attribute to no value.
*
*                 item_labels    Optional. If the names of the attribute values to
*                                be shown to the user needs to be different from
*                                the actual values as stored in the items parameter
*                                above, this is where this can be done. The number
*                                of item_labels needs to be the same length as the
*                                number of items.
*
*                 none_sub       If one of the items is "none" this is the string
*                                to substitute. The user will see this substituted
*                                string but its action will be the same as "none".
*                                Only valid if item_labels is not defined.
*
*                 default        The item from the above list to be used as the
*                                default value.
*
*                 limit          If the allowed values in the list are limited
*                                by some other entry object, this key defines the
*                                relationship. See note above.
*
*                 limit_type     One of "integer", "float" or "string". This has
*                                meaning only if a limit is specified and determines
*                                if the limit testing is numeric or by string
*                                comparison. Default is integer.
*
*  radio_buttons  attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. In either
*                                case the set of buttons is displayed inside a
*                                frame. If set to "none" then no frame is put
*                                around the buttons.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 items          The list of attribute values to appear in the
*                                radio buttons, separated by white space. If any
*                                item must have embedded spaces, then it must be
*                                enclosed in quotes. There is a special item with
*                                a name of "none" which can be inserted into the
*                                list and when selected will set the current value
*                                of the corresponding attribute to no value.
*
*                 item_labels    Optional. If the names of the attribute values to
*                                be shown to the user needs to be different from
*                                the actual values as stored in the items parameter
*                                above, this is where this can be done. The number
*                                of item_labels needs to be the same length as the
*                                number of items.
*
*                 none_sub       If one of the items is "none" this is the string
*                                to substitute. The user will see this substituted
*                                string but its action will be the same as "none".
*                                Only valid if item_labels is not defined.
*
*                 default        The item from the items list to be used as the
*                                default value.
*
*
*  scrolled_list  attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 ncolumns       The size of the object specified in character
*                                columns.
*
*                 items          The list of attribute values to appear in the
*                                list, separated by white space. If any item must
*                                have embedded spaces, then it must be enclosed
*                                in quotes. There is a special item with a name
*                                of "none" which can be inserted into the list
*                                and when selected will set the current value of
*                                the corresponding attribute to no value.
*
*                 item_labels    Optional. If the names of the attribute values to
*                                be shown to the user needs to be different from
*                                the actual values as stored in the items parameter
*                                above, this is where this can be done. The number
*                                of item_labels needs to be the same length as the
*                                number of items.
*
*                 none_sub       If one of the items is "none" this is the string
*                                to substitute. The user will see this substituted
*                                string but its action will be the same as "none".
*                                Only valid if item_labels is not defined.
*
*                 nvisible       The number of items which will be visible in
*                                the scrolled list area. Default is the number
*                                of items in the list.
*
*                 default        The item from the above list to be used as the
*                                default value.
*
*                 limit          If the allowed values in the list are limited
*                                by some other entry object, this key defines the
*                                relationship. See note above.
*
*                 limit_type     One of "integer", "float" or "string". This has
*                                meaning only if a limit is specified and determines
*                                if the limit testing is numeric or by string
*                                comparison. Default is integer.
*
*
*  spin_box       attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry.
*
*                 ncolumns       The number of columns that will be used by the
*                                spin_box to display the value.
*
*                 value_max      The maximum value that a value may be.
*
*                 value_min      The minimum value of a value.
*
*                 default        The value which will appear in the spin_box.
*                 
*                 increment      The amount to increment the value by.
*                                Defaults to 1.
*                 
*                 wrap           Either "True" of "False". When the value in the
*                                spin_box reaches the maximum or minimum does it
*                                spin around to the next value. If, for example,
*                                the spin_box reaches the maximum, then a wrap value
*                                of True means that it will spin around to the
*                                minumum. Default False.
*
*
*  spin_list      attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 ncolumns       The size of the object specified in character
*                                columns.
*
*                 items          The list of items to appear in the list.
*
*                 none_sub       Substitute the given string for the "none"
*                                entry in the list. The user will see this
*                                substituted string but its action will be the
*                                same as "none". Default is a blank (" ").
*
*                 items          The list of attribute values to appear in the
*                                radio buttons, separated by white space. If any
*                                item must have embedded spaces, then it must be
*                                enclosed in quotes. There is a special item with
*                                a name of "none" which can be inserted into the
*                                list and when selected will set the current value
*                                of the corresponding attribute to no value.
*
*                 item_labels    Optional. If the names of the attribute values to
*                                be shown to the user needs to be different from
*                                the actual values as stored in the items parameter
*                                above, this is where this can be done. The number
*                                of item_labels needs to be the same length as the
*                                number of items.
*
*                 none_sub       If one of the items is "none" this is the string
*                                to substitute. The user will see this substituted
*                                string but its action will be the same as "none".
*                                Only valid if item_labels is not defined.
*
*                 default        The item from the above list to be used as the
*                                default value.
*
*                 limit          If the allowed values in the list are limited
*                                by some other entry object, this key defines the
*                                relationship. See note above.
*
*                 limit_type     One of "integer", "float" or "string". This has
*                                meaning only if a limit is specified and determines
*                                if the limit testing is numeric or by string
*                                comparison. Default is integer.
*                 
*                 wrap           Either "True" of "False". When the value in the
*                                spin_list reaches the maximum or minimum does it
*                                spin around to the next value. If, for example,
*                                the spin_list reaches the maximum, then a wrap value
*                                of True means that it will spin around to the
*                                minumum. Default False.
*
*        
*  composite_list attribute_id   The name of the attribute to be controlled by
*                                this composite list object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 border_width   The width of the border around the label.
*                                Default 0.
*
*                 border_color   The colour of the border.
*
*                 margin_height  The height of the margin, in pixels, above and
*                                below the child objects. Default 0.
*
*                 margin_width   The width of the margin, in pixels, to the
*                                right and left of the child objects. Default 0.
*
*                 The objects to be part of the composite follow these
*                 parameters.
*
*                 For example:
*
*                   composite_list
*                   {
*                       attribute_id = cloud_base
*                       position     = 73, 127
*                       ....
*
*                       popup_list
*                       {
*                       	composite_id cb1
*                           ....
*                       }
*                       popup_list
*                       {
*                       	composite_id cb2
*                       	prefix       dash
*                           ....
*                       }
*                   }
*
*                 The list of items that make up the individual parts of the
*                 composite must be specified as the item list in each child
*                 object.
*
*                 None of the child objects requires their attribute_id keyword
*                 to be specified and it is ignored if present. In addition all
*                 of the children of the composite_list can have the following
*                 keywords in addition to their normal keyword set:
* 
*                 composite_id - If a child needs to make reference to another
*                                child, such as is required by the "limit"
*                                keyword, this key provides the means to give
*                                the referenced child an identifier that can be
*                                used as the reference id. If an object outside
*                                of the composite list needs to refer to an
*                                object in the composite it can do so by using
*                                the fully qualified identifier
*                                	attribute_id:composite_id
*                                In the above example this could be
*                                   cloud_base:cb2 
*
*                 prefix       - A character that will be inserted before that
*                                part of the composite entry controlled by the
*                                child object. If specified and the character
*                                is not a space, then the composite object will
*                                display the character before the child object.
*                                There are two special entries here that can be
*                                used for clarity: "space" which denotes the
*                                use of a space as a prefix and "dash" which
*                                denotes the use of a dash "-". All other cases
*                                are taken as literal.
*
*
*  text           attribute_id   The name of the attribute to be controlled by
*                                this object.
*
*                 label          If not set, then the label of the list is the
*                                attribute label. If set, the given value is
*                                used instead of the attribute label. If set to
*                                "none" then no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 ncolumns       The horizontal size of the text area in
*                                columns. Defaults to 10.
*
*                 nrows          The vertical size of the text area in rows.
*                                Defaults to 1.
*
*                 default        A string to initialize the attribute with.
*
*
* - Control Objects -
*
*  clear_btn      label          Optional. The label to display to the user.
*                                (Default "<< Clear".)
*
*                 position       The offset, as x and y in pixels, of the button
*                                from the upper left corner of its parent which
*                                can be either a panel or frame.
*
*                 foreground     The foreground colour of the label.
*                                Default parent's.
*
*                 background     The background colour of the label.
*                                Default parent's.
*
*                 border_width   The width of the border around the label.
*                                Default 0.
*
*                 border_color   The colour of the border.
*
*                 margin_height  The height of the margin, in pixels, above and
*                                below the string in the label. Default 2.
*
*                 margin_width   The width of the margin, in pixels, to the
*                                right and left of the string in the label.
*                                Default 2.
*
*                 attribute_id_list  A list of attribute ids which are to be
*                                    reset to their default state when this
*                                    button is pressed.
*
*
*  run_program    label          The label to display on the button.
*
*                 position       The offset, as x and y in pixels, of the
*                                button from the upper left corner of its
*                                parent, which can be either a panel or frame.
*                                
*                 size           The width and height of the button, in pixels,
*                                if you do not want the button default size.
*
*                 foreground     The foreground colour of the label.
*                                Default parent's.
*
*                 background     The background colour of the label.
*                                Default parent's.
*
*                 border_width   The width of the border around the label.
*                                Default 0.
*
*                 border_color   The colour of the border.
*
*                 margin_height  The height of the margin, in pixels, above and
*                                below the string in the label. Default 2.
*
*                 margin_width   The width of the margin, in pixels, to the
*                                right and left of the string in the label.
*                                Default 2.
*
*                 program        The name of the program to run.
*
*                 missing        The string to pass the program if any of the
*                                parameter values is missing or not available.
*                                The default if not specified is "none".
*
*                 parms          The run time parameters of the program. All
*                                entries are taken as literal except for
*                                special key words given in angle backets
*                                <key>. These are substituted for the actual
*                                values when the button is activated.
*                                The keys available are:
*
*                                <SETUP>       - The name of the setup file.
*                                <RTIME>       - Run time. For depictions this
*                                                 is T0.
*                                <VTIME>       - Valid time.
*                                <ELEMENT>     - Currently active element.
*                                <LEVEL>       - Currently active level.
*                                <LATITUDE>    - Latitude of point or label.
*                                <LONGITUDE>   - Longitude of point or label.
*                                <ATTRIB:name> - The value of the attribute
*                                                 "name".
*
*                                Latitude and longitude are only available for
*                                point fields and labels.
*                                
*                                One could have as an entry in the parms line
*                                items like "latitude=<latitude>". 
*
*
* - Display Only Objects -
*
*  display_label  label          The label to display to the user.
*
*                 position       The offset, as x and y in pixels, of the label
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 foreground     The foreground colour of the label.
*                                Default parent's.
*
*                 background     The background colour of the label.
*                                Default parent's.
*
*                 border_width   The width of the border around the label.
*                                Default 0.
*
*                 border_color   The colour of the border.
*
*                 margin_height  The height of the margin, in pixels, above and
*                                below the string in the label. Default 2.
*
*                 margin_width   The width of the margin, in pixels, to the
*                                right and left of the string in the label.
*                                Default 2.
*
*
*  display_attribute
*
*                 attribute_id   The name of the attribute to be displayed by
*                                this object.
*
*                 label          If not set, then the label is the attribute
*                                label. If set, the given value is used instead
*                                of the attribute label. If set to "none" then
*                                no label is displayed.
*
*                 label_size     Either "short" or "long". Used if the label
*                                above is not set and the label is taken from
*                                the attribute. In this case there is a long
*                                form and short form attribute label available.
*                                Default "long"
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 ncolumns       The horizontal size of the text area in
*                                characters. Defaults to 10.
*
*
*  line           line_style     One of "etched_in" (default), "etched_out",
*                                "single", "double", "single_dashed" or
*                                "double_dashed".
*
*                 orientation    Either "horizontal" (default) or "vertical".
*
*                 position       The offset, as x and y in pixels, of the entry
*                                object from the upper left corner of its
*                                parent which can be either a panel or frame.
*
*                 length         The length of the line in pixels.
*
*                 foreground     The foreground colour of the line.
*                                Default parent's.
*
*                 background     The background colour of the line.
*                                Default parent's.
*
*
*
*
*
*============== EXAMPLE MENU FILE =================
*
* menu_title           =  Weather Entry Menu
* geometry             =  -650 -300 700 600
* label_display_height =  1
* value_display_height =  3
*
* panel "Cloud"
* {
*    # First cloud layer - base, cover, top
*    composite_list
*    {
*        attribute_id   = cloud_base
*        label          = Cloud
*        position       = 20 30
*        default        = 10
*        button_list
*        {
*            composite_id = cb1
*            ncolumns     = 3
*            nvisible     = 10
*            none_sub     = ""
*            items        = none 10 20 30 40 50 60 70 80 90 100 \
*                            110 120 130 140 150 160 170 180 190 200 \
*                            210 220 230 240 250 260 270 280 290 300 \
*                            310 320 330 340 350 360 370 380 390 400
*        }
*        button_list
*        {
*            ncolumns     = 3
*            nvisible     = 10
*            none_sub     = ""
*            prefix       = dash
*            limit        = GT cb1
*            limit_type   = integer
*            items        = none 10 20 30 40 50 60 70 80 90 100 \
*                            110 120 130 140 150 160 170 180 190 200 \
*                            210 220 230 240 250 260 270 280 290 300 \
*                            310 320 330 340 350 360 370 380 390 400
*        }
*    }
*    composite_list
*    {
*        attribute_id   = cloud_amount
*        label          = none
*        position       = 215 30
*        default        = SCT
*        scrolled_list
*        {
*            ncolumns = 4
*            none_sub = CLR
*            items    = none SCT BKN OVC
*        }
*        scrolled_list
*        {
*            prefix   = space
*            ncolumns = 5
*            nvisible = 3
*            none_sub = ""
*            items    = none OCNL VRBL
*        }
*        scrolled_list
*        {
*            prefix   = space
*            ncolumns = 4
*            none_sub = None
*            items    = none SCT BKN OVC
*        }
*        scrolled_list
*        {
*            prefix   = space
*            ncolumns = 5
*            none_sub = None
*            items    = none CU TCU CB LYRS
*        }
*    }
*    scrolled_list
*    {
*        attribute_id = cloud_top
*        label        = none
*        position     = 530 30
*        default      = 50
*        item_type    = integer
*        ncolumns     = 3
*        none_sub     = ""
*        items        = none 10 20 30 40 50 60 70 80 90 100 \
*                            110 120 130 140 150 160 170 180 190 200 \
*                            210 220 230 240 250 260 270 280 290 300 \
*                            310 320 330 340 350 360 370 380 390 400
*    }
*    frame
*    {
*        label    = Frame
*        position = 20 180
*        size     = 600 80
*        text
*        {
*            attribute_id  = cloud_remarks
*            position      = 10 10
*            ncolumns      = 40
*            nrows         = 2
*        }
*    }
* }
* panel "Weather"
* {
*    composite_list
*    {
*        attribute_id   = weather
*        position       = 20 30
*        default        = none
*        scrolled_list
*        {
*            ncolumns     = 3
*            none_sub     = Unl
*            items        = none 0 1/4 1/2 3/4 1 2 3 4 5 6 P6
*        }
*        scrolled_list
*        {
*            prefix       = dash
*            ncolumns     = 4
*            none_sub     = ""
*            items        = none 1/4 1/2 3/4 1 2 3 4 5 6 P6
*        }
*        scrolled_list
*        {
*            ncolumns     = 3
*            none_sub     = ""
*            items        = SM none
*        }
*        scrolled_list
*        {
*            prefix       = space
*            ncolumns     = 1
*            none_sub     = ""
*            items        = none "+" "-"
*        }
*        scrolled_list
*        {
*            ncolumns     = 5
*            none_sub     = None
*            items        = none RA SHRA SN SHSN DZ FZRA FZDZ
*        }
*        scrolled_list
*        {
*            prefix       = space
*            ncolumns     = 6
*            none_sub     = None
*            items        = none BR FG BR/FG
*        }
*    }
*
* ----------------------------------------------------------------------------
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*/
/*=====================================================================================*/

#include "global.h"
#include <math.h>
#include <ctype.h>
#if (XmVERSION >= 2)
#include <Xm/GrabShell.h>
#endif
#include <Xm/ComboBox.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/XmpSpinBox.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/TabStack.h>
#include "depiction.h"
#include "editor.h"
#include "help.h"

#define NONE				"none"
#define DEFAULT_NONE_LABEL	" "
#define BUFSIZE				512		/* read file buffer size */
#define NITEMS_LIMIT		10000	/* Maximum number of items per object allowed */

#define DIALOG_NAME			"attributesEntryMenu"

#define OBJECT_BTNLIST		"button_list"
#define OBJECT_CLEAR_BTN	"clear_btn"
#define OBJECT_COMPOSITE	"composite_list"
#define OBJECT_DPY_ATTRIB	"display_attribute"
#define OBJECT_FRAME		"frame"
#define OBJECT_LABEL		"display_label"
#define OBJECT_LINE			"line"
#define OBJECT_PANEL		"panel"
#define OBJECT_POPUP		"popup_list"
#define OBJECT_RADIO_BTNS	"radio_buttons"
#define OBJECT_RUN_PGM		"run_program"
#define OBJECT_SCROLL		"scrolled_list"
#define OBJECT_SPINBOX		"spinbox"
#define OBJECT_SPIN_BOX		"spin_box"		/* to avoid problems */
#define OBJECT_SPINLIST		"spin_list"
#define OBJECT_TEXT			"text"

/* Obsolete keys kept for backwards compatability */
#define OBJECT_DPY_ONLY     "display_only"	/* same as OBJECT_DPY_ATTRIB */

/* These defines reference the key words in key_list
 * and are used to increase code meaning. If any new
 * keys are added then both this list and key_list
 * must be modified.
 */
#define KEY_ATTRIB_ID		key_list[0]
#define KEY_ATTRIB_ID_LIST	key_list[1]
#define KEY_BACKGROUND		key_list[2]
#define KEY_BORDER_COLOR    key_list[3]
#define KEY_BORDER_WIDTH    key_list[4]
#define KEY_BUTTON_LOOK		key_list[5]
#define KEY_CATEGORY_COLS	key_list[6]
#define KEY_COMPOSITE_ID	key_list[7]
#define KEY_DEFAULT			key_list[8]
#define KEY_DEFAULT_ATTRIB	key_list[9]
#define KEY_DISPLAY_TYPE    key_list[10]
#define KEY_DIALOG_TITLE    key_list[11]
#define KEY_DOUBLE			key_list[12]
#define KEY_DOUBLE_DASH		key_list[13]
#define KEY_ETCHED_IN		key_list[14]
#define KEY_ETCHED_OUT		key_list[15]
#define KEY_FOREGROUND		key_list[16]
#define KEY_GEOMETRY		key_list[17]
#define KEY_INCREMENT       key_list[18]
#define KEY_INITIALIZATION	key_list[19]
#define KEY_INITIALISATION	key_list[20]
#define KEY_ITEMS			key_list[21]
#define KEY_ITEM_LABELS		key_list[22]
#define KEY_LABEL			key_list[23]
#define KEY_LABEL_FONT		key_list[24]
#define KEY_LABEL_HEIGHT	key_list[25]
#define KEY_LABEL_SIZE		key_list[26]
#define KEY_LENGTH			key_list[27]
#define KEY_LIMIT			key_list[28]
#define KEY_LIMIT_TYPE		key_list[29]
#define KEY_LINE_STYLE		key_list[30]
#define KEY_LIST			key_list[31]
#define KEY_LIST_FONT		key_list[32]
#define KEY_MARGIN_WIDTH    key_list[33]
#define KEY_MARGIN_HEIGHT   key_list[34]
#define KEY_MISSING			key_list[35]
#define KEY_NCOLS			key_list[36]
#define KEY_NONE_SUB		key_list[37]
#define KEY_NROWS			key_list[38]
#define KEY_NVISIBLE		key_list[39]
#define KEY_ORIENTATION		key_list[40]
#define KEY_PARMS           key_list[41]
#define KEY_POSN			key_list[42]
#define KEY_PREFIX			key_list[43]
#define KEY_PROGRAM         key_list[44]
#define KEY_SINGLE			key_list[45]
#define KEY_SINGLE_DASH		key_list[46]
#define KEY_SIZE			key_list[47]
#define KEY_SPACING         key_list[48]
#define KEY_TYPE 			key_list[49]
#define KEY_VALUE_ALIGNMENT	key_list[50]
#define KEY_VALUE_HEIGHT	key_list[51]
#define KEY_VALUE_MAX       key_list[52]
#define KEY_VALUE_MIN       key_list[53]
#define KEY_WRAP			key_list[54]
#define KEY_ITEM_TYPE		key_list[55]	/* obsolete - for compatability only */

/* The type of dialog currently active. This code does not allow for multiple
 * instances of the entry menu to exist at the same time. The code associated
 * with the following will destroy the menu if a creation function is called
 * when a menu of a different type is currently active.
 */
enum { TYPE_NONE, TYPE_POINT, TYPE_LABEL, TYPE_LINE, TYPE_AREA, TYPE_AREAB, TYPE_LCHAIN, TYPE_LNODE, ENTRY_TYPE };

/* useful defines */
#define ISNONE(a)    (blank(a)||same_ic(a,NONE))
#define CV(a)        (ISNONE(a->value)?CAL_NO_VALUE:a->value)
#define STRVAL(a)    (ISNONE(a)?DEFAULT_NONE_LABEL:a)
#define BUFEND(a)    a[(*a)?(strlen(a)-1):0]

/* For the exit_cb function */
#define EXIT_NORMAL			(XtPointer)0
#define EXIT_CANCEL			(XtPointer)1


typedef void (*MAKE_ENTRY_FCN)(CAL);

typedef enum { PrefixNone, PrefixSpace } PREFIX;
typedef enum { LimNone, LimLT, LimLE, LimGE, LimGT, LimAND, LimOR } LIMIT_LOGIC;
typedef enum { LimTypeNone, LimTypeString, LimTypeInteger, LimTypeFloat } LIMIT_TYPE;
typedef enum { OvTypeNone, OvTypeValue, OvTypeAttrib } OVERRIDE_TYPE;

/* Object types */
typedef enum {
	Generic,
	Btnlist,
	Popup,
	RadioButtons,
	Scroll,
	Spinbox,
	Spinlist,
	Text,
	Label,
	Composite,
	Table,
	Viewer,
	PgmRunBtn
} OBJECT_TYPE;


/* Data structures
 */
typedef struct {
	String        data;				/* used to hold pointer to menu data */
	LIMIT_TYPE    type;				/* numeric or string comparison */
	struct _entry *entry[2];		/* the limiting objects */
	LIMIT_LOGIC   select[2];		/* ge, le, gt, lt */
	LIMIT_LOGIC   logic;			/* and, or */
} LIMIT_STRUCT;

typedef struct _entry {
	String        id;				/* attribute_id or composite_id of object */
	Widget        w;				/* selection list object */
	OBJECT_TYPE   type;				/* type of object */
	OVERRIDE_TYPE override_type;	/* OvTypeValue or OvTypeAttrib */
	String        default_override;	/* if not NULL the menu file default override */
	String        none_sub;			/* "none" substitute */
	String        value;			/* active value */
	LIMIT_STRUCT  *limit;			/* value limiting information */
	int           nitems;			/* number of value choices */
	String        *items;			/* value choices */
	String        *ilabs;			/* item labels - what the user sees */
	int           nlist;			/* length of selectable item list */
	String        *list;			/* restricted subset of items for selection */
	int           nvis;				/* number of items to be visible in the list */
	char          prefix[2];		/* prefix the list values with this character */
	struct _entry *comp_parent;	    /* non-null only if parent is a composite */
	int           nchild;			/* number of child objects of a composite */
	struct _entry **child_entry;	/* entry of the child objects of a composite */
} ENTRY_STRUCT, *ENTRY;

/* used with the btnlist object to hold the popup list info */
typedef struct {
	Widget popup;
	Widget list;
	ENTRY  entry;
} BtnList;

/* Local functions */
static void    block_end                 (void);
static void    build_composite_attribute (ENTRY);
static void    build_dialog              (Widget, String, CAL, XuDialogActionsStruct*, int, String);
static void    check_for_value_limit     (ENTRY, const Boolean);
static void    clear_label_text_cb       (Widget, XtPointer, XtPointer);
static void    create_btnlist            (void);
static void    create_separator          (void);
static void    create_clear_btn          (void);
static void    create_viewer             (void);
static void    create_tab_body           (void);
static void    create_frame              (void);
static void    create_display_label      (void);
static void    create_popup_list         (void);
static void    create_radio_buttons      (void);
static void    create_run_pgm_btn        (void);
static void    create_scrolled_list      (void);
static void    create_spinbox            (void);
static void    create_spin_list          (void);
static void    create_text               (void);
static void    create_composite          (void);
static void    enter_cb                  (Widget, XtPointer, XtPointer);
static void    enter_and_exit_cb         (Widget, XtPointer, XtPointer);
static void    exit_cb                   (Widget, XtPointer, XtPointer);
static String  get_next_key              (void);
static Boolean have_menu_file            (String);
static void    key_error                 (String, String);
static Boolean get_key_data              (String, String*);
static int     key_value                 (String, int);
static void    key_values                (String, int*, int*);
static Boolean key_boolean               (String, const Boolean);
static void    label_text_cb             (Widget, XtPointer, XtPointer);
static void    reset_cb                  (Widget, XtPointer, XtPointer);
static void    scrolled_list_cb          (Widget, XtPointer, XtPointer);
static void    set_last_drawn_cb         (Widget, XtPointer, XtPointer);
static Boolean set_composite_values      (ENTRY, String);
static void    set_limiting_attributes   (void);
static void    set_objects_to_cal_values (CAL, String);
static void    text_verify_cb            (Widget, XtPointer, XtPointer);
static void    update_text_displays      (CAL);

/* This is the pointer to the function which invokes the rules for the field
 * The rules are either entry or label types and are determined by the type
 * of function called to create the entry menu.
 */
static void (*invoke_rules)(CAL,String,String) = CAL_invoke_entry_rules_by_name;

/* Static variables
 */
static FIELD_INFO *active_entry_field    = (FIELD_INFO*)NULL;
static Widget     dialog                 = NullWidget;
static Widget     labelDisplay           = NullWidget;
static Widget     categoryDisplay        = NullWidget;
static Widget     valueDisplay           = NullWidget;
static Widget     setLastDrawnBtn        = NullWidget;
static int        ntabs                  = 0;
static Widget     tabs                   = NullWidget;
static WidgetList tabPages               = 0;
static Boolean    multitabs              = False;
static int        nentry                 = 0;
static ENTRY      entry                  = NULL;
static Boolean    lock_label_display     = False;
static Boolean    updating_label_display = False;
static CAL        cal                    = (CAL)0;
static CAL        incal                  = (CAL)0;
static int        depth                  = 0;
static int        nparents               = 0;
static WidgetList parents                = (WidgetList)NULL;
static String     *labels                = NULL;
static Boolean    initializing           = True;
static String     *keys                  = NULL;
static String     *key_data              = NULL;
static int        datalenmax             = 0;
static int        datalen                = 0;
static int        cpos                   = 0;
static int        fpos                   = 0;
static String     module                 = NULL;
static String     menu_file              = NULL;
static int        type_active            = TYPE_NONE;

/* Key string used in parsing the menu file. Any additions
 * to this list should also be reflected in the set of defines
 * above
 */
const String key_list[] = 
{
	"attribute_id",
	"attribute_id_list",
	"background",
	"border_color",
	"border_width",
	"button_look",
	"category_display_width",
	"composite_id",
	"default",
	"default_to_attribute",
	"display_type",
	"menu_title",
	"double",
	"double_dash",
	"etched_in",
	"etched_out",
	"foreground",
	"geometry",
	"increment",
	"initialization",
	"initialisation",    /* For the Americans */
	"items",
	"item_labels",
	"label",
	"label_font",
	"label_display_height",
	"label_size",
	"length",
	"limit",
	"limit_type",
	"line_style",
	"list",
	"list_font",
	"margin_width",
	"margin_height",
	"missing",
	"ncolumns",
	"none_sub",
	"nrows",
	"nvisible",
	"orientation",
	"parms",
	"position",
	"prefix",
	"program",
	"single",
	"single_dash",
	"size",
	"spacing",
	"type",
	"value_alignment",
	"value_display_height",
	"value_max",
	"value_min",
	"wrap",
	"item_type"		/* obsolete - same as limit_type */
};

/* Messages that may be used in more than one place
 */
const String obsolete_key_msg = "Obsolete keyword \"%s\" in menu file \"%s\". Replace with \"%s\"\n";



/*================== Externally Visible Functions ======================*/



void ACTIVATE_labelAttributesDialog(Widget w, CAL in_cal, String menu, void (*active_fcn)())
{
    static XuDialogActionsStruct action_items[] = {
		{ "setBtn",    enter_and_exit_cb, NULL           },
		{ "resetBtn",  reset_cb,          NULL           },
		{ "cancelBtn", exit_cb,           EXIT_CANCEL    },
		{ "helpBtn",   HelpCB,            HELP_GFA_ENTRY }
	};

	module = "LabelAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_LABEL )
	{
		DestroyAttributesEntryDialog();
		XmUpdateDisplay(w);
	}

	/* Of course all rules for labels must be label rules ;-) */
	invoke_rules = CAL_invoke_label_rules_by_name;

	if(IsNull(dialog))
	{
		menu_file = menu;
		action_items[0].data = (XtPointer) active_fcn;
		if(have_menu_file(menu_file))
		{
			type_active = TYPE_LABEL;
			build_dialog(w, LABEL_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), "setBtn");
		}
		else
		{
			pr_error(module,
				"Invalid attributes entry menu definition file : \"%s\" for label %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}


void ACTIVATE_pointAttributesDialog( Widget w, CAL in_cal, void (*active_fcn)() )
{
	int    n;
	String type;
	FpaConfigElementScatteredTypeStruct *stypes;

    static XuDialogActionsStruct action_items[] = {
		{ "setBtn",    enter_and_exit_cb, NULL           },
		{ "resetBtn",  reset_cb,          NULL           },
		{ "cancelBtn", exit_cb,           EXIT_CANCEL    },
		{ "helpBtn",   HelpCB,            HELP_GFA_ENTRY }
	};

	module = "PointAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_POINT )
	{
		DestroyAttributesEntryDialog();
		XmUpdateDisplay(w);
	}

	/* All rules for point data types are label rules */
	invoke_rules = CAL_invoke_label_rules_by_name;

	if(IsNull(dialog))
	{
		action_items[0].data  = (XtPointer)active_fcn;

		type   = CAL_get_attribute(in_cal, CALscatteredtype); 
		stypes = GV_active_field->info->element->elem_detail->scattered_types;

		for(n = 0; n < stypes->ntypes; n++)
		{
			if(!same(type,stypes->type_names[n])) continue;
			if(InEditMode(E_MODIFY))
				menu_file = stypes->type_modify_files[n];
			else
				menu_file = stypes->type_entry_files[n];
			break;
		}

		/* Since we can not pre-test the existance of menu files for labels we will
		*  assume that no menu file here means that this is deliberate and just exit
		*  without complaint.
		*/
		if(blank(menu_file)) return;

		if( have_menu_file(menu_file) )
		{
			type_active = TYPE_POINT;
			build_dialog(w, POINT_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), NULL);
		}
		else
		{
			pr_error(module,
				"Invalid attributes entry menu definition file : \"%s\" for point field %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}



void ACTIVATE_lineAttributesDialog( Widget w, CAL in_cal, void (*active_fcn)() )
{
	FpaConfigElementEditorStruct *editor = GV_active_field->info->element->elem_detail->editor;

    static XuDialogActionsStruct action_items[] = {
		{ NULL,             set_last_drawn_cb,NULL           },
		{ "setDefaultBtn",  enter_cb,         NULL           },
		{ "resetMenuBtn",   reset_cb,         NULL           },
		{ "closeBtn",       exit_cb,          EXIT_NORMAL    },
		{ "helpBtn",        HelpCB,           HELP_GFA_ENTRY }
	};

	module = "LineAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_LINE )
	{
		DestroyAttributesEntryDialog();
		XmUpdateDisplay(w);
	}

	invoke_rules = CAL_invoke_entry_rules_by_name;

	if(IsNull(dialog))
	{
		action_items[1].data  = (XtPointer)active_fcn;

		if(InEditMode(E_MODIFY))
		{
			action_items[0].id = "setSelectedLineBtn";
			menu_file = editor->type.line->modify_file;
		}
		else
		{
			action_items[0].id = "setLastLineBtn";
			menu_file = editor->type.line->entry_file;
		}

		if( have_menu_file(menu_file) )
		{
			type_active = TYPE_LINE;
			build_dialog(w, LINE_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), NULL);
		}
		else
		{
			pr_error(module,
				"Invalid attributes entry menu definition file : \"%s\" for line field %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}



void ACTIVATE_areaAttributesDialog( Widget w, CAL in_cal, void (*active_fcn)(), void(*store_fcn)() )
{
	FpaConfigElementEditorStruct *editor = GV_active_field->info->element->elem_detail->editor;

    static XuDialogActionsStruct action_items[] = {
		{ NULL,             set_last_drawn_cb,NULL           },
		{ "setDefaultBtn",  enter_cb,         NULL           },
		{ "addToListBtn",   enter_cb,         NULL           },
		{ "resetMenuBtn",   reset_cb,         NULL           },
		{ "closeBtn",       exit_cb,          EXIT_NORMAL    },
		{ "helpBtn",        HelpCB,           HELP_GFA_ENTRY }
	};

	module = "AreaAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_AREA )
	{
		DestroyAttributesEntryDialog();
		XmUpdateDisplay(w);
	}

	invoke_rules = CAL_invoke_entry_rules_by_name;

	if(IsNull(dialog))
	{
		action_items[1].data  = (XtPointer)active_fcn;
		action_items[2].data  = (XtPointer)store_fcn;

		if(InEditMode(E_MODIFY))
		{
			action_items[0].id = "setSelectedAreaBtn";
			menu_file = editor->type.discrete->modify_file;
		}
		else if(InEditMode(E_DIVIDE))
		{
			action_items[0].id = "setSubAreaBtn";
			menu_file = editor->type.discrete->entry_file;
		}
		else
		{
			action_items[0].id = "setLastAreaBtn";
			menu_file = editor->type.discrete->entry_file;
		}

		if( have_menu_file(menu_file) )
		{
			type_active = TYPE_AREA;
			build_dialog(w, AREA_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), NULL);
		}
		else
		{
			pr_error(module,
				"Invalid attributes entry menu definition file : \"%s\" for area field %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}


void ACTIVATE_areaBkgndAttributesDialog( Widget w, CAL in_cal, void (*active_fcn)())
{
	FpaConfigElementEditorStruct *editor = GV_active_field->info->element->elem_detail->editor;

    static XuDialogActionsStruct action_items[] = {
		{ "setLastAreaBtn", set_last_drawn_cb, NULL           },
		{ "setDefaultBtn",  enter_and_exit_cb, NULL           },
		{ "resetMenuBtn",   reset_cb,          NULL           },
		{ "cancelBtn",      exit_cb,           EXIT_CANCEL    },
		{ "helpBtn",        HelpCB,            HELP_GFA_ENTRY }
	};

	module = "AreaBackgroundAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_AREAB )
	{
		DestroyAttributesEntryDialog();
		XmUpdateDisplay(w);
	}

	invoke_rules = CAL_invoke_entry_rules_by_name;

	if(IsNull(dialog))
	{
		action_items[1].data = (XtPointer)active_fcn;

		menu_file = editor->type.discrete->back_entry_file;

		if( have_menu_file(menu_file) )
		{
			type_active = TYPE_AREAB;
			build_dialog(w, AREA_BKGND_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), NULL);
		}
		else
		{
			pr_error(module,
				"Invalid background attributes entry menu definition file : \"%s\" for area field %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}



void ACTIVATE_linkChainAttributesDialog( Widget w, CAL in_cal, void (*active_fcn)() )
{
	FpaConfigElementEditorStruct *editor = GV_active_field->info->element->elem_detail->editor;

    static XuDialogActionsStruct action_items[] = {
		{ "setBtn",    enter_and_exit_cb, NULL                     },
		{ "resetBtn",  reset_cb,          NULL                     },
		{ "cancelBtn", exit_cb,           EXIT_CANCEL              },
		{ "helpBtn",   HelpCB,            HELP_LCHAIN_ATTRIB_ENTRY }
	};

	module = "LinkChainAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_LCHAIN )
		DestroyAttributesEntryDialog();

	/* All rules for point data types are label rules */

	if(IsNull(dialog))
	{
		action_items[0].data  = (XtPointer)active_fcn;

		invoke_rules = CAL_invoke_entry_rules_by_name;
		if(InEditMode(E_MODIFY))
			menu_file = editor->type.lchain->modify_file;
		else
			menu_file = editor->type.lchain->entry_file;

		if( have_menu_file(menu_file) )
		{
			type_active = TYPE_LCHAIN;
			build_dialog(w, LCHAIN_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), NULL);
		}
		else
		{
			pr_error(module,
				"Invalid attributes entry menu definition file : \"%s\" for link chain field %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}



void ACTIVATE_linkNodeAttributesDialog( Widget w, CAL in_cal, void (*active_fcn)() )
{
	FpaConfigElementEditorStruct *editor = GV_active_field->info->element->elem_detail->editor;

    static XuDialogActionsStruct action_items[] = {
		{ "setBtn",    enter_and_exit_cb, NULL                    },
		{ "resetBtn",  reset_cb,          NULL                    },
		{ "cancelBtn", exit_cb,           EXIT_CANCEL             },
		{ "helpBtn",   HelpCB,            HELP_LNODE_ATTRIB_ENTRY }
	};

	module = "LinkNodeAttributesEntryMenu";

	if( NotNull(dialog) && type_active != TYPE_LNODE )
		DestroyAttributesEntryDialog();

	/* All rules for point data types are label rules */

	if(IsNull(dialog))
	{
		action_items[0].data  = (XtPointer)active_fcn;

		invoke_rules = CAL_invoke_lnode_rules_by_name;
		if(InEditMode(E_MODIFY))
			menu_file = editor->type.lchain->node_modify_file;
		else
			menu_file = editor->type.lchain->node_entry_file;

		if( have_menu_file(menu_file) )
		{
			type_active = TYPE_LNODE;
			build_dialog(w, LNODE_ATTRIBUTES_DIALOG, in_cal, action_items, (int) XtNumber(action_items), NULL);
		}
		else
		{
			pr_error(module,
				"Invalid attributes entry menu definition file : \"%s\" for link nodes of link chain field %s\n",
				menu_file, GV_active_field->info->element->name);
		}
	}
	else
	{
		set_objects_to_cal_values(in_cal, NULL);
	}
	XuShowDialog(dialog);
}



/* Function to determine if the menu files for the given field as defined
*  in the configuration files actually exist and are accessable. The attrib_type
*  key are bit set macros that can be or'ed together to return the state of
*  more than one type.
*/
Boolean MenuFileExists( int attrib_type, FIELD_INFO *field )
{
	String  mf;
	Boolean entry_ok = False, modify_ok = False, bkgnd_ok = False, node_ok = False;
	String  fmt = "Data entry dialog %s file \"%s\" defined in config file for field \"%s\" is inaccessible\n";

	/* Check for null elements in the chain */
	if (!field) return False;
	if (!field->info) return False;
	if (!field->info->element) return False;
	if (!field->info->element->elem_detail) return False;
	if (!field->info->element->elem_detail->editor) return False;
	/* type is a union so we only need to check one member */
	if (!field->info->element->elem_detail->editor->type.discrete) return False;

	if(field->info->element->fld_type == FpaC_DISCRETE)
	{
		if(attrib_type & ENTRY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.discrete->entry_file;
			if(!blank(mf))
			{
				entry_ok = have_menu_file(mf);
				if (!entry_ok) pr_error(module, fmt, "entry", mf, field->info->element->name);
			}
		}
		if(attrib_type & MODIFY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.discrete->modify_file;
			if(!blank(mf))
			{
				modify_ok = have_menu_file(mf);
				if (!modify_ok) pr_error(module, fmt, "modify", mf, field->info->element->name);
			}
		}
		if(attrib_type & BACKGROUND_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.discrete->back_entry_file;
			if(!blank(mf))
			{
				bkgnd_ok = have_menu_file(mf);
				if (!bkgnd_ok) pr_error(module, fmt, "background entry", mf, field->info->element->name);
			}
		}
		return (entry_ok || modify_ok || bkgnd_ok);
	}
	else if(field->info->element->fld_type == FpaC_LINE)
	{
		if(attrib_type & ENTRY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.line->entry_file;
			if(!blank(mf))
			{
				entry_ok = have_menu_file(mf);
				if (!entry_ok) pr_error(module, fmt, "entry", mf, field->info->element->name);
			}
		}
		if(attrib_type & MODIFY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.line->modify_file;
			if(!blank(mf))
			{
				modify_ok = have_menu_file(mf);
				if (!modify_ok) pr_error(module, fmt, "modify", mf, field->info->element->name);
			}
		}
		return (entry_ok || modify_ok);
	}
	else if(field->info->element->fld_type == FpaC_LCHAIN)
	{
		if(attrib_type & ENTRY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.lchain->entry_file;
			if(!blank(mf))
			{
				entry_ok = have_menu_file(mf);
				if (!entry_ok) pr_error(module, fmt, "entry", mf, field->info->element->name);
			}
		}
		if(attrib_type & MODIFY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.lchain->modify_file;
			if(!blank(mf))
			{
				modify_ok = have_menu_file(mf);
				if (!modify_ok) pr_error(module, fmt, "modify", mf, field->info->element->name);
			}
		}
		if(attrib_type & NODE_ENTRY_ATTRIBUTES)
		{
			mf = field->info->element->elem_detail->editor->type.lchain->node_entry_file;
			if(!blank(mf))
			{
				node_ok = have_menu_file(mf);
				if (!node_ok) pr_error(module, fmt, "entry", mf, field->info->element->name);
			}
		}

		if( (attrib_type & ENTRY_ATTRIBUTES) || (attrib_type & MODIFY_ATTRIBUTES) )
			return (entry_ok || modify_ok);

		if(attrib_type & NODE_ENTRY_ATTRIBUTES)
			return (node_ok);

	}
	return False;
}


/* Public function called to update the entry menu lists to a given
*  CAL structure and/or set the sensitivity state of the button which
*  will apply the active cal structure to the last area drawn.
*  Only active if the entry field set in initialization is the same
*  as the current active field.
*/
void UpdateAttributesEntryDialog(CAL in_cal)
{
	if(IsNull(dialog)) return;

	if( setLastDrawnBtn )
	{
		Boolean show = ((XtIsSensitive(GW_editorAcceptBtn) && (InEditMode(E_DRAW) || InEditMode(E_ADD))) || InEditMode(E_MODIFY) || InEditMode(E_DIVIDE)); 
		XtSetSensitive(setLastDrawnBtn, show);
			
	}

	if( NotNull(in_cal) && active_entry_field == GV_active_field )
	{
		CAL_empty(cal);
		CAL_merge(cal, in_cal, True);
		CAL_empty(incal);
		CAL_merge(incal, in_cal, True);
		lock_label_display = False;
		set_objects_to_cal_values(cal, NULL);
		XuShowDialog(dialog);
	}
}


/* Public function to destroy this dialog when called.
*/
void DestroyAttributesEntryDialog(void)
{
	exit_cb(NULL, EXIT_NORMAL, NULL);
}



/*========================== Private Functions ============================*/



/* Returns true only if the given menu file exists.
 */
static Boolean have_menu_file(String fname)
{
	String path;

	if(blank(fname)) return False;

	path = get_file(MENUS_CFG,fname);
	if(blank(path)) return False;

	return (access(path, R_OK) == 0);
}


/*ARGSUSED*/
static void set_object_values_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	set_objects_to_cal_values((CAL)client_data, NULL);
}


/*  Create the attribute element entry menu. If not NULL the default_item parameter defines the
 *  action item button that is set as the default.
 */
static void build_dialog(Widget ref_widget, String profile_name, CAL in_cal, 
							XuDialogActionsStruct *action_items, int naction_items, String default_item)
{
	int      i, nlists, ncategory_cols, nlabel_rows, nvalue_rows;
	String   line, key, ptr, menufile, data;
	Boolean  is_label, is_category, is_value, data_initialize, obsolete_menu;
	Widget   rc, btn, label, form, top_widget;
	Pixel    bkgnd;
	FILE     *fp;
	XmString dialog_title = NULL;
	void     (*action_fcn)(void) = NULL;

	/* Get the full path name of the menu file
	 */
	menufile = get_file(MENUS_CFG,menu_file);

	/* Capture the active field as we don't want this to change while this
	*  panel is active (as it could).
	*/
	active_entry_field = GV_active_field;

	/* These are used to determine the number of tabs and entry objects
	 * required to layout the entry menu, construct the tab label string
	 * and determine the position and display height.
	*/
	ntabs           = 0;
	nlists          = 0;
	depth           = 1;
	nparents        = 2;
	labelDisplay    = NullWidget;
	categoryDisplay = NullWidget;
	valueDisplay    = NullWidget;
	data_initialize = True;

	/* Create our menu file data buffer. This is done as it is easier to put the
	*  menu information into a buffer than to work with the file directly.
	*/
	fpos       = 0;
	cpos       = 0;
	datalenmax = 0;
	datalen    = 0;
	keys       = NULL;
	key_data   = NULL;

	fp = fopen(menufile, "r");
	if(!fp)
	{
		pr_error(module, "Unable to open attributes entry menu config file \"%s\"\n", menufile);
		return;
	}

	while((line = ReadLine(fp)))
	{
		if(datalen >= datalenmax)
		{
			keys     = MoreMem(keys,     String, (datalenmax += 100));
			key_data = MoreMem(key_data, String, datalenmax);
		}
		/*
		 * Parse out the keys. As the keys are non-space separated words remove
		 * the '=' and parse with the standard functions.
		 */
		if((ptr = strchr(line, '='))) *ptr = ' ';
		key = string_arg(line);
		if(blank(key)) continue;
		lower_case(key);
		keys[datalen] = XtNewString(key);
		/*
		 * A line can end with a comment that starts with a '#'
		 */
		if((ptr = strchr(line,'#'))) *ptr = '\0';
		no_white(line);
		key_data[datalen] = (blank(line))? NULL:XtNewString(line);
		datalen++;
	}
	(void) fclose(fp);

	/* Scan the data buffer to find out the number of panels we will need and
	*  how many levels of parents we will have. This way we can allocate the
	*  needed objects up front instead of on the fly. This simplifies code.
	*  Objects that do not result in an entry in the list of objects are used
	*  as well so that we can check for unknown object entries.
	*/
	obsolete_menu = False;
	while((key = get_next_key()) && !obsolete_menu)
	{
		if(*key == '{')
		{
			depth++;
			nparents = MAX(depth, nparents);
		}
		else if(*key == '}')
		{
			depth--;
		}
		else if(same(key, OBJECT_BTNLIST   )) {nlists++;}
		else if(same(key, OBJECT_CLEAR_BTN )) {/* does not add to list */}
		else if(same(key, OBJECT_COMPOSITE )) {nlists++;}
		else if(same(key, OBJECT_DPY_ATTRIB)) {nlists++;}
		else if(same(key, OBJECT_FRAME     )) {/* does not add to list */}
		else if(same(key, OBJECT_LABEL     )) {/* does not add to list */}
		else if(same(key, OBJECT_LINE      )) {/* does not add to list */}
		else if(same(key, OBJECT_POPUP     )) {nlists++;}
		else if(same(key, OBJECT_RUN_PGM   )) {/* does not add to list */}
		else if(same(key, OBJECT_SCROLL    )) {nlists++;}
		else if(same(key, OBJECT_SPINBOX   )) {nlists++;}
		else if(same(key, OBJECT_SPIN_BOX  )) {nlists++;}
		else if(same(key, OBJECT_SPINLIST  )) {nlists++;}
		else if(same(key, OBJECT_TEXT      )) {nlists++;}
		else if(same(key, OBJECT_RADIO_BTNS)) {nlists++;}
		else if(same(key, OBJECT_PANEL     ))
		{
			labels = MoreMem(labels, String, ntabs+1);
			if(blank(key_data[cpos-1]))
			{
				labels[ntabs] = AllocPrint("tab%d", ntabs);
			}
			else
			{
				char buf[BUFSIZE];
				(void) strncpy(buf, key_data[cpos-1], BUFSIZE);
				ptr = string_arg(buf);
				labels[ntabs] = XtNewString(ptr);
			}
			ntabs++;
		}
		else if(same(key,OBJECT_DPY_ONLY))
		{
			String msg = "Obsolete object \"%s\" in menu file \"%s\". Replace with object \"%s\"\n";
			pr_warning(module, msg, OBJECT_DPY_ONLY, menu_file, OBJECT_DPY_ATTRIB);
			nlists++;
		}
		else
		{
			/* Check to see if the key is recognized */
			for( i = 0; i < (int) XtNumber(key_list); i++ )
				if(same(key, key_list[i])) break;

			if(i >= (int) XtNumber(key_list))
			{
				/* If not found check to ensure that it is not one of the keys
				 * associated with the obsolete version of the composite list.
				 * If so terminate the menu creation process.
				 */
				if(sscanf(key, "list%d_", &i) == 1)
					obsolete_menu = True;
				else
					pr_error(module, "Unrecognized keyword \"%s\" in menu file \"%s\"\n", key, menu_file);
			}
		}
	}

	if(obsolete_menu)
	{
		pr_error(module, "Menu file \"%s\" contains an obsolete composite object definition.\n", menu_file);
		pr_error(module, "Menu creation terminated. Please contact your system administrator for help.\n");
		FreeList(keys, datalen);
		FreeList(key_data, datalen);
		FreeList(labels, ntabs);
		(void) get_key_data(NULL,NULL);
		return;
	}

	multitabs = (ntabs > 1);
	tabPages = NewMem(Widget, ntabs);

	/* If we didn't find any panels we have a serious error!
	*/
	if(ntabs == 0)
	{
		pr_error(module, "No panels defined in menu config file \"%s\"\n", menufile);
		FreeList(keys, datalen);
		FreeList(key_data, datalen);
		return;
	}


	cpos = 0;
	if(get_key_data(KEY_DIALOG_TITLE, &data))
	{
		/* If the title is in quotes use string_arg() to strip them */
		if(strchr("\"\'", *data)) data = string_arg(data);
		dialog_title = XmStringCreateLocalized(data);
	}
	if(get_key_data(KEY_GEOMETRY, &data))
	{
		String items[] = {"x","y","width","height"};
		for(i = 0; i < 4; i++)
		{
			char buf[BUFSIZE];
			if(!(key = string_arg(data))) break;
			(void) snprintf(buf, sizeof(line), "%s*%s.%s", GV_app_name, DIALOG_NAME, items[i]);
			XuPutStringResource(buf, key);
		}
	}
	if(get_key_data(KEY_INITIALIZATION, &data) || get_key_data(KEY_INITIALISATION, &data))
	{
		key = string_arg(data);
		if(same_ic(key, NONE)) data_initialize = False;
	}
	nlabel_rows    = key_value(KEY_LABEL_HEIGHT, 1);
	nvalue_rows    = key_value(KEY_VALUE_HEIGHT, 2);
	ncategory_cols = key_value(KEY_CATEGORY_COLS, 10);

	block_end();

	/* Create and fill in the cal data objects depending on the initialization key.
	*/
	cal = CAL_create_by_edef(active_entry_field->info->element);
	if(data_initialize)
	{
		incal = CAL_duplicate(in_cal);
		CAL_merge(cal, incal, True);
	}
	else
	{
		incal = CAL_duplicate(cal);
	}

	/* Create the dialogID for this particular instance of the dialog. Since this can
	 * have several instances associated with different fields, the id is complex with
	 * the second part adding granularity. The part before XuDIALOG_ID_PART_SEPARATOR
	 * is used to set the dialog title and the entire complex is for dialog geometry.
	 */
	ptr = AllocPrint("%s%s%s%s%s", profile_name, XuDIALOG_ID_PART_SEPARATOR, menu_file,
			active_entry_field->info->element->name, active_entry_field->info->level->name);

	dialog = XuCreateToplevelFormDialog(ref_widget, DIALOG_NAME,
		XmNdialogTitle, dialog_title,
		XuNdialogID, ptr,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, naction_items,
		XuNdefaultActionItemId, default_item,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		XuNmwmDeleteOverride, exit_cb,
		XuNmwmDeleteData, EXIT_CANCEL,
		NULL);

	FreeItem(ptr);
	if (dialog_title) XmStringFree(dialog_title);

	/* We want those items which are not allowed to be changed to be visually
	 * different from those that can be changed. This is done by setting the
	 * background colour of the non-editable text fields to the background
	 * colour of the dialog.
	 */
	XtVaGetValues(dialog, XmNbackground, &bkgnd, NULL);

	setLastDrawnBtn = NullWidget;
	if( same(action_items[0].id, "setLastAreaBtn")     ||
		same(action_items[0].id, "setSelectedAreaBtn") ||
		same(action_items[0].id, "setLastLineBtn")     ||
		same(action_items[0].id, "setSelectedLineBtn")   )
	{
		Boolean show = ((XtIsSensitive(GW_editorAcceptBtn) && (InEditMode(E_DRAW) || InEditMode(E_ADD))) || InEditMode(E_MODIFY) || InEditMode(E_DIVIDE)); 
		setLastDrawnBtn = XuGetActionAreaBtn(dialog, action_items[0]);
		XtSetSensitive(setLastDrawnBtn, show);
	}

	is_label    = (nlabel_rows > 0);
	is_value    = (nvalue_rows > 0);
	is_category = (ncategory_cols > 0);

	if(is_label)
	{
		label = XmVaCreateManagedLabel(dialog, "labelDisplayLabel",
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		btn = XmVaCreateManagedPushButton(dialog, "labelDisplayClear",
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 0,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
		XtAddCallback(btn, XmNactivateCallback, clear_label_text_cb, NULL);

		labelDisplay = XmVaCreateManagedText(dialog, "labelDisplay",
			XmNeditMode, XmMULTI_LINE_EDIT,
			XmNwordWrap, True,
			XmNrows, nlabel_rows,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 0,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_WIDGET,
			XmNrightWidget, btn,
			NULL);
		XtAddCallback(labelDisplay, XmNmodifyVerifyCallback, text_verify_cb, NULL);
		XtAddCallback(labelDisplay, XmNvalueChangedCallback, label_text_cb,  (XtPointer)0);
		XtAddCallback(labelDisplay, XmNlosingFocusCallback,  label_text_cb,  (XtPointer)1);
	}

	if(is_category)
	{
		form = XmVaCreateForm(dialog, "form",
			XmNtopAttachment, (is_label && is_value) ? XmATTACH_WIDGET : XmATTACH_FORM,
			XmNtopWidget, labelDisplay,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		label = XmVaCreateManagedLabel(form, "category",
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		categoryDisplay = XmVaCreateManagedTextField(form, "categoryDisplay",
			XmNcursorPositionVisible, False,
			XmNeditable, False,
			XmNbackground, bkgnd,
			XmNcolumns, (short) ncategory_cols,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		XtManageChild(form);

		if(is_label && !is_value)
			XtVaSetValues(btn,
				XmNrightAttachment, XmATTACH_WIDGET,
				XmNrightWidget, form,
				NULL);
	}

	if(is_value)
	{
		label = XmVaCreateManagedLabel(dialog, "valueDisplayLabel",
			XmNtopAttachment, is_label ? XmATTACH_WIDGET : XmATTACH_FORM,
			XmNtopWidget, labelDisplay,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		valueDisplay = XmVaCreateManagedText(dialog, "valueDisplay",
			XmNeditable, False,
			XmNeditMode, XmMULTI_LINE_EDIT,
			XmNwordWrap, True,
			XmNrows, nvalue_rows,
			XmNautoShowCursorPosition, False,
			XmNcursorPositionVisible, False,
			XmNbackground, bkgnd,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 0,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, is_category ? XmATTACH_WIDGET:XmATTACH_FORM,
			XmNrightWidget, form,
			XmNrightOffset, is_category? 19:9,
			NULL);
	}

	parents     = NewMem(Widget, nparents);
	entry       = NewMem(ENTRY_STRUCT, nlists);
	parents[0]  = dialog;
	depth       = 0;
	nentry      = 0;
	top_widget  = NullWidget;

	if (is_category) top_widget = form;
	if (is_label   ) top_widget = labelDisplay;
	if (is_value   ) top_widget = valueDisplay;

	/* Create the button which allow the user to see the original unaltered
	*  attributes as they came in from the area(s).
	*/
	rc = XmVaCreateManagedPushButton(dialog, "originalValues",
		XmNmarginHeight, 5,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(rc, XmNarmCallback,    set_object_values_cb, (XtPointer)incal);
	XtAddCallback(rc, XmNdisarmCallback, set_object_values_cb, (XtPointer)cal  );

	if(NotNull(top_widget))
	{
		XtVaSetValues(rc, 
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, top_widget,
			XmNtopOffset, 9,
			NULL);
	}

	if(multitabs)
	{
		tabs = XmVaCreateManagedTabStack(dialog, "tabs",
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, rc,
			XmNtopOffset, 0,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	}
	else
	{
		parents[1] = XmVaCreateManagedBulletinBoard(dialog, labels[0],
			XmNshadowThickness, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, rc,
			XmNtopOffset, 9,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	}


	/* The following actually creates the attribute data display and entry objects.
	*/
	ntabs = 0;
	while((ptr = get_next_key()))
	{
		if     (same(ptr, "}"              )) depth--;
		else if(same(ptr, "{"              )) action_fcn();
		else if(same(ptr, OBJECT_BTNLIST   )) action_fcn = create_btnlist;
		else if(same(ptr, OBJECT_CLEAR_BTN )) action_fcn = create_clear_btn;
		else if(same(ptr, OBJECT_COMPOSITE )) action_fcn = create_composite;
		else if(same(ptr, OBJECT_DPY_ONLY  )) action_fcn = create_viewer;
		else if(same(ptr, OBJECT_DPY_ATTRIB)) action_fcn = create_viewer;
		else if(same(ptr, OBJECT_FRAME     )) action_fcn = create_frame;
		else if(same(ptr, OBJECT_LABEL     )) action_fcn = create_display_label;
		else if(same(ptr, OBJECT_LINE      )) action_fcn = create_separator;
		else if(same(ptr, OBJECT_PANEL     )) action_fcn = create_tab_body;
		else if(same(ptr, OBJECT_POPUP     )) action_fcn = create_popup_list;
		else if(same(ptr, OBJECT_RADIO_BTNS)) action_fcn = create_radio_buttons;
		else if(same(ptr, OBJECT_RUN_PGM   )) action_fcn = create_run_pgm_btn;
		else if(same(ptr, OBJECT_SCROLL    )) action_fcn = create_scrolled_list;
		else if(same(ptr, OBJECT_SPINBOX   )) action_fcn = create_spinbox;
		else if(same(ptr, OBJECT_SPIN_BOX  )) action_fcn = create_spinbox;
		else if(same(ptr, OBJECT_SPINLIST  )) action_fcn = create_spin_list;
		else if(same(ptr, OBJECT_TEXT      )) action_fcn = create_text;
		else 
		{
			pr_warning(module, "Unrecognized block identifier \"%s\" in menu file \"%s\"\n",
				ptr, menufile);
			while((ptr = get_next_key()) != NULL && !same(ptr,"}"));
		}
	}

	/* All limits are processed after all of the objects have been created as it is
	 * quite legal to forward reference objects as value limiters. This function also
	 * uses the data buffers and thus must be called before the buffers are freed.
	 */
	set_limiting_attributes();

	/* release the menu file data buffers
	 */
	FreeList(keys, datalen);
	FreeList(key_data, datalen);
	FreeList(labels, ntabs);
	(void) get_key_data(NULL,NULL);

	/* Manage the composite objects and set the composite default.
	 */
	for( i = 0; i < nentry; i++ )
	{
		if(entry[i].type != Composite) continue;
		if(!set_composite_values(&entry[i], entry[i].default_override))
			pr_error(module,"Default value \"%s\" in menu file \"%s\" is invalid.\n",
					entry[i].default_override, menufile);
		XtManageChild(entry[i].w);
	}
	
	/* Initialize the object values
	 */
	set_objects_to_cal_values(cal, NULL);
}


/*ARGSUSED*/
static void label_text_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Only respond if the user is doing the update */
	if (updating_label_display) return;

	/* client_data will be non-zero if this is from a losing focus callback */
	if(PTR2INT(client_data))
	{
		/* If lock_label_display is false then the display string must be blank
		 * so set it to the auto label
		 */
		if(!lock_label_display)
		{
			updating_label_display = True;
			XmTextSetString(labelDisplay, CAL_get_attribute(cal,CALautolabel));
			updating_label_display = False;
		}
	}
	else
	{
		/* Value changed. If string is not blank then the user entered something */
		String str = XmTextGetString(w);
		lock_label_display = !blank(str);
		FreeItem(str);
	}
}


/*ARGSUSED*/
static void clear_label_text_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if(IsNull(labelDisplay)) return;
	updating_label_display = True;
	XmTextSetString(labelDisplay, CAL_get_attribute(cal,CALautolabel));
	updating_label_display = False;
	lock_label_display = False;
}


/*ARGSUSED*/
static void reset_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int    i;
	String value;

	CAL_empty(cal);
	CAL_merge(cal, incal, True);
	for(i = 0; i < nentry; i++)
	{
		if(IsNull(entry[i].comp_parent) && NotNull(entry[i].default_override))
		{
			value = CAL_get_attribute(cal, XtName(entry[i].w));
			if(CAL_no_value(value))
			{
				if( entry[i].override_type == OvTypeValue )
				{
					CAL_set_attribute(cal, XtName(entry[i].w), entry[i].default_override);
				}
				else
				{
					value = CAL_get_attribute(cal, entry[i].default_override);
					CAL_set_attribute(cal, XtName(entry[i].w), value);
				}
			}
		}
	}
	set_objects_to_cal_values(cal, NULL);
}



/* Return the next key in the key buffer and increment
 * the current position pointer.
 */
static String get_next_key(void)
{
	String k = NULL;
	if(cpos < datalen)
	{
		k = keys[cpos];
		cpos++;
	}
	return k;
}


/* General error function for invalue keys and key values
 */
static void key_error(String key, String value)
{
	if (value)
		pr_error(module, "Invalid value \"%s\" for key \"%s\" in file: %s\n", value, key, menu_file);
	else
		pr_error(module, "Invalid value for key \"%s\" in file: %s\n", key, menu_file);
}


/* Search for the given key in the current object block and read the
 * data line in the current object block which starts with the given key.
 * The data line pointer is returned.
 */
static Boolean get_key_data(String key, String *value)
{
	String k;

	/* A string long enough to hold the longest key_data[] element */
	static String key_data_buffer = NULL;

	/* If key is NULL free the buffer */
	if(!key)
	{
		FreeItem(key_data_buffer);
		return False;
	}

	if(!key_data_buffer)
	{
		int i, len;
		for( len = 0, i = 0; i < datalen; i++)
			len = MAX(len, safe_strlen(key_data[i]));
		key_data_buffer = NewMem(char,len+2);
	}

	(void) strcpy(key_data_buffer, "");
	cpos = fpos;
	while((k = get_next_key()))
	{
		if( *k == '{' || *k == '}' ) break;
		if(same_ic(key, k))
		{
			(void) safe_strcpy(key_data_buffer, key_data[cpos-1]);
			break;
		}
	}
	if (value) *value = key_data_buffer;
	return !blank(key_data_buffer);
}


/* Return the integer value associated with the given key. If
 * the key is not found or if there is an error return the
 * default value.
 */
static int key_value(String key, int default_value)
{
	String data;
	int  val = default_value;
	if(get_key_data(key, &data))
	{
		Boolean ok;
		int n = int_arg(data, &ok);
		if (ok) val = n;
		else    key_error(key, NULL);
	}
	return val;
}


/* Return the pair of values associated with the given key.
 */
static void key_values(String key, int *x, int *y)
{
	String data;
	if(get_key_data(key, &data))
	{
		Boolean ok;

		int n = int_arg(data, &ok);
		if (ok) *x = n;
		else    key_error(key, NULL);

		n = int_arg(data, &ok);
		if (ok) *y = n;
		else    key_error(key, NULL);
	}
}


/* Return a true or false associated with the given key. Note
 * that no and yes are taken as the same as false and true.
 */
static Boolean key_boolean(String key, const Boolean default_return)
{
	String data;
	if(get_key_data(key, &data))
	{
		String ptr = string_arg(data);
		if(same_start_ic(ptr,"t") || same_start_ic(ptr,"y"))
			return True;
		else if(same_start_ic(ptr,"f") || same_start_ic(ptr,"n"))
			return False;
		else
			key_error(key, ptr);
	}
	return default_return;
}



/* A fully qualified object id for a composite object consists of the
 * attribute of the composite plus the id of the individual composite
 * object. Thus we could have cloud_base:cb1 for example. This function
 * creates the full id.
 */
static String make_full_id( ENTRY ep, String id )
{
	if(NotNull(strchr(id,':')) || IsNull(ep->comp_parent))
		return AllocPrint("%s", id);
	else
		return AllocPrint("%s:%s", ep->comp_parent->id, id);
}



/* Create the entry for the given object type.
 *
 * Check to see if the parent is a composite type. Note that the number of objects
 * that check for membership in the composite is limited. If a member of a composite
 * has a prefix character and that character is not a space, then a label object is
 * created as a child of the composite that will show the prefix before the object.
 *
 * Get the attribute id of the object.
 *
 * Set the default override if it exists.
 *
 */
static void initialize_entry( OBJECT_TYPE type, ENTRY *ep, Boolean *attrib_found )
{
	int   n;
	char  *p, *s;
	String data;
	ENTRY e;

	const String deferr = "Menu file %s. Keywords %s and %s both present in entry object %s.\n";

	e = &entry[nentry];
	nentry++;

	(void)memset((void*)e, 0, sizeof(ENTRY_STRUCT));

	e->type          = type;
	e->override_type = OvTypeNone;

	*attrib_found = True;

	/* Find the none substitute value if any. Note that any null or blank
	 * input is treated as a single blank as defined in DEFAULT_NONE_LABEL.
	 * This is required by many lists in order for them to display properly.
	 */
	p = DEFAULT_NONE_LABEL;
	if(get_key_data(KEY_NONE_SUB, &data))
	{
		s = string_arg(data);
		if (!blank(s)) p = s;
	}
	e->none_sub = XtNewString(p);

	/* look for a prefix character
	 */
	if(get_key_data(KEY_PREFIX, &data))
	{
		String ptr = string_arg(data);
		if(same_ic(ptr,"space"))
			*e->prefix = ' ';
		else if(same_ic(ptr,"blank"))
			*e->prefix = ' ';
		else if(same_ic(ptr,"dash"))
			*e->prefix = '-';
		else
			*e->prefix = *ptr;
	}

	/* Check to see if the parent is a Composite object */
	for( n = 0; n < nentry; n++ )
	{
		if( entry[n].w == parents[depth] && entry[n].type == Composite )
		{
			if(type == Scroll || type == Popup || type == Spinlist || type == Btnlist )
			{
				entry[n].child_entry = MoreMem(entry[n].child_entry, ENTRY, entry[n].nchild+1);
				entry[n].child_entry[entry[n].nchild] = e;
				entry[n].nchild++;
				e->comp_parent = &entry[n];
			}
			if(*e->prefix)
			{
				(void)XmVaCreateManagedLabel(parents[depth], e->prefix, XmNborderWidth, 0, NULL);
			}
			break;
		}
		else if( entry[n].w == parents[depth] && entry[n].type == Table )
		{
			break;
		}
	}
	*ep = e;

	/* Now get the entry->id which is the attribute name of this object.
	 */
	if(e->comp_parent)
	{
		/* Composite id's replace the attribute for those objects that make up the
		 * composite. As this is optional we do not warn on a missing entry but make
		 * up our own. Make the id the fully qualified one.
		 */
		if(get_key_data(KEY_COMPOSITE_ID, &data))
		{
			e->id = make_full_id(e, data);
		}
		else
		{
			char buf[24];
			(void) snprintf(buf, sizeof(buf),  "CLC%d", nentry);
			e->id = make_full_id(e, buf);
		}
	}
	else
	{
		if(get_key_data(KEY_ATTRIB_ID, &data))
		{
			String sa = string_arg(data);
			e->id = XtNewString((sa)? sa : "???");
			if(!CAL_has_attribute(cal, e->id))
			{
				*attrib_found = False;
				pr_error(module, "Invalid attribute \"%s\" in menu file \"%s\".\n", e->id, menu_file);
			}
		}
		else
		{
			e->id  = XtNewString("attribute");
			*attrib_found = False;
			pr_error(module, "Missing %s in menu file \"%s\"\n", KEY_ATTRIB_ID, menu_file);
		}
	}

	/* Set default override
	 */
	if(get_key_data(KEY_DEFAULT_ATTRIB, &data))
	{
		if(e->override_type != OvTypeNone)
		{
			pr_error(module, deferr, menu_file, KEY_DEFAULT_ATTRIB, KEY_DEFAULT, e->id);
		}
		else
		{
			e->override_type = OvTypeAttrib;
			p = string_arg(data);
			if(!CAL_has_attribute(cal, p))
			{
				pr_error(module, "Unrecognized attribute \"%s\" in menu file \"%s\".\n", p, menu_file);
			}
			else
			{
				String value = CAL_get_attribute(cal, e->id);
				if(CAL_no_value(value))
				{
					value = CAL_get_attribute(cal, p);
					CAL_set_attribute(cal, e->id, value);
				}
				e->default_override = XtNewString(p);
			}
		}
	}

	if(get_key_data(KEY_DEFAULT, &data))
	{
		if(e->override_type != OvTypeNone)
		{
			pr_error(module, deferr, menu_file, KEY_DEFAULT, KEY_DEFAULT_ATTRIB, e->id);
		}
		else
		{
			/* Bugfix 20050627.1 - allow a no value default */
			e->override_type = OvTypeValue;
			p = string_arg(data);
			if(ISNONE(p))
			{
				p = CAL_NO_VALUE;
			}
			else if(CAL_no_value(CAL_get_attribute(cal, e->id)))
			{
				CAL_set_attribute(cal, e->id, p);
			}
			e->default_override = XtNewString(p);
		}
	}
}


/* Look for the limit key, create the limit structure and hold
 * the pointer to the limit data. The data will be parsed by the
 * set_limiting_attributes function after all of the objects
 * have been created.
 */
static void find_value_limit(ENTRY e)
{
	if(get_key_data(KEY_LIMIT,NULL))
	{
		String p, data;

		e->limit = OneMem(LIMIT_STRUCT);
		e->limit->data      = key_data[cpos-1];
		e->limit->entry[0]  = NULL;
		e->limit->entry[1]  = NULL;
		e->limit->select[0] = LimNone;
		e->limit->select[1] = LimNone;
		e->limit->logic     = LimNone;
		e->limit->type      = LimTypeInteger;

		if(get_key_data(KEY_LIMIT_TYPE, &data))
		{
			p = string_arg(data);
			if(     same_start_ic(p,"s")) e->limit->type = LimTypeString;
			else if(same_start_ic(p,"i")) e->limit->type = LimTypeInteger;
			else if(same_start_ic(p,"f")) e->limit->type = LimTypeFloat;
			else key_error(KEY_LIMIT_TYPE, p);
		}
		else if(get_key_data(KEY_ITEM_TYPE, &data))
		{
			pr_warning(module, obsolete_key_msg, KEY_ITEM_TYPE, menu_file, KEY_LIMIT_TYPE);
			p = string_arg(data);
			if(     same_start_ic(p,"s")) e->limit->type = LimTypeString;
			else if(same_start_ic(p,"i")) e->limit->type = LimTypeInteger;
			else if(same_start_ic(p,"f")) e->limit->type = LimTypeFloat;
			else key_error(KEY_LIMIT_TYPE, p);
		}
	}
}


/* Set the current data buffer position to the current pointer position.
 */
static Boolean block_start(void)
{
	fpos = cpos;
	if(NotNull(parents[depth])) return True;
	block_end();
	return False;
}


/* Set the current data buffer position to the end of the object block ready
*  for reading of the next block identification key. If we find a '}' the
*  pointer is already at the next line. If we find a '{' we need to decrement
*  the pointer to be set to the line before the '{' line.
*/
static void block_end(void)
{
	cpos = fpos;
	while(cpos < datalen)
	{
		if(*(keys[cpos]) == '}') {cpos++; return;}
		if(*(keys[cpos]) == '{') {cpos--; return;}
		cpos++;
	}
}



/* Find a label. If the data line starts with a quote, find the matching
 * quote and remove both and return the XmString version. This allows the
 * use of both quoted and unquoted strings. Since the label is on a line
 * by itself, this is a reasonable thing to do.
 */
static Boolean label_found( XmString *label )
{
	char *ptr, *ls;
	String data;

	if(!get_key_data(KEY_LABEL, &data)) return False;

	ls = data;
	if(*data == '"')
	{
		ptr = strchr(data+1,'"');
		if (ptr)
		{
			*ptr = '\0';
			ls++;
		}
	}
	if(blank(ls)) return FALSE;

	*label = XmStringCreateLocalized(ls);
	return True;
}



/* Given the attribute name return the corresponding label. This searches for
*  the key "label_size =" in the object block and expects to find either "long"
*  or 'short" as the value. (It actually only checks for a starting 's'). The
*  default is to return a long form label but will return the short form if the
*  short key is found. We first check for a LABEL key. If this is set then use
*  this instead of the attribute name. if "none" then return NULL.
*/
static void find_attribute_label(ENTRY e, XmString *xmlabel)
{
	int     i;
	String data;
	XmString xm_label;
	FpaConfigElementAttribStruct *ap;

	static XmString xm_none  = (XmString)0;
	static XmString xm_space = (XmString)0;

	if (!xm_none ) xm_none  = XmStringCreateLocalized(NONE);
	if (!xm_space) xm_space = XmStringCreateLocalized(" ");

	*xmlabel = (XmString)0;

	if( e->comp_parent ) return;

	if(label_found(&xm_label))
	{
		if(XmStringCompare(xm_label, xm_none))
		{
			XmStringFree(xm_label);
		}
		else
		{
			*xmlabel = XmStringConcat(xm_label,xm_space);
			XmStringFree(xm_label);
		}
	}
	else
	{
		ap = active_entry_field->info->element->elem_detail->attributes;
		for(i = 0; i < ap->nattribs; i++)
		{
			if(same(e->id, ap->attrib_names[i]))
			{
				if(get_key_data(KEY_LABEL_SIZE, &data) && same_start_ic(data,"s"))
					xm_label = XmStringCreateLocalized(ap->attrib_sh_labels[i]);
				else
					xm_label = XmStringCreateLocalized(ap->attrib_labels[i]);
				*xmlabel = XmStringConcat(xm_label, xm_space);
				XmStringFree(xm_label);
				return;
			}
		}
		*xmlabel = XmStringCreateLocalized("??? ");
	}
}



static void set_position( ArgList al, Cardinal *ac )
{
	int x = 0, y = 0;

	key_values(KEY_POSN, &x, &y);

	XtSetArg(al[*ac], XmNx, x); (*ac)++;
	XtSetArg(al[*ac], XmNy, y); (*ac)++;
}


static void set_border( ArgList al, Cardinal *ac )
{
	String data;
	int  width = key_value(KEY_BORDER_WIDTH,-1);

	if(width >= 0)
	{
		XtSetArg(al[*ac], XmNborderWidth, (Dimension)width); (*ac)++;
	}
	if (get_key_data(KEY_BORDER_COLOR, &data))
	{
		XtSetArg(al[*ac], XmNborderColor, XuLoadColor(dialog, string_arg(data))); (*ac)++;
	}
}


/* The default_width and default_height are applied if there is no override in the
 * menu file and if they have a value of >= 0, otherwise the value is not set.
 */
static void set_margins( ArgList al, Cardinal *ac, int default_width, int default_height  )
{
	int width  = key_value(KEY_MARGIN_WIDTH ,-1);
	int height = key_value(KEY_MARGIN_HEIGHT,-1);

	if(width >= 0)
	{
		XtSetArg(al[*ac], XmNmarginWidth, (Dimension)width); (*ac)++;
	}
	else if(default_width >= 0)
	{
		XtSetArg(al[*ac], XmNmarginWidth, (Dimension)default_width); (*ac)++;
	}

	if(height >= 0)
	{
		XtSetArg(al[*ac], XmNmarginHeight, (Dimension)height); (*ac)++;
	}
	else if(default_height >= 0)
	{
		XtSetArg(al[*ac], XmNmarginHeight, (Dimension)default_height); (*ac)++;
	}
}

static void set_colours( ArgList al, Cardinal *ac )
{
	String data;
	if(get_key_data(KEY_BACKGROUND, &data))
	{
		XtSetArg(al[*ac], XmNbackground, XuLoadColor(dialog, string_arg(data))); (*ac)++;
	}
	if(get_key_data(KEY_FOREGROUND, &data))
	{
		XtSetArg(al[*ac], XmNforeground, XuLoadColor(dialog, string_arg(data))); (*ac)++;
	}
}



/* Look for a key that returns the alignment of the value string
 * in the object. Valid values are left, center and right.
 */
static void set_alignment( ArgList al, Cardinal *ac )
{
	String data;
	if(get_key_data(KEY_VALUE_ALIGNMENT, &data))
	{
		String ptr = string_arg(data);

		if( same_start_ic(ptr,"L"))
		{
			XtSetArg(al[*ac], XmNalignment, XmALIGNMENT_BEGINNING); (*ac)++;
		}
		else if(same_start_ic(ptr,"R"))
		{
			XtSetArg(al[*ac], XmNalignment, XmALIGNMENT_END); (*ac)++;
		}
		else if(same_start_ic(ptr,"C"))
		{
			XtSetArg(al[*ac], XmNalignment, XmALIGNMENT_CENTER); (*ac)++;
		}
		else
		{
			key_error(KEY_VALUE_ALIGNMENT, ptr);
		}
	}
}


/* Given an item, return the associated item label. If label is recognized
 * as a tstamp then display it in the format set in the resource file for
 * dates.
 */
static String item_label(ENTRY e, String item)
{
	int    n;
	String label = item;

	for(n = 0; n < e->nitems; n++)
	{
		if(!same(item,e->items[n])) continue;
		label = e->ilabs[n];
		break;
	}

	/* Output an error only if there is an entry item list to scan */
	if(label == item && e->nitems > 0)
		pr_error(module, "Unable to get label for unknown item = \"%s\"\n", item);

	return DateString(label, DEFAULT_FORMAT);
}


static String value_label(ENTRY e)
{
	return item_label(e,e->value);
}


/* Get the labels associated with the restricted list of items. Note that 
 * the list of labels array is internally static and should not be freed.
 */
static String *list_labels(ENTRY e)
{
	int n, pos;
	static String *ll = NULL;

	ll = MoreMem(ll, String, e->nlist);
	for(n = 0; n < e->nlist; n++)
	{
		ll[n] = (InList(e->list[n],e->nitems,e->items,&pos))? e->ilabs[pos]:"???";
	}
	return ll;
}


/* Assign a attribute to the entry value. The ENTRY item is used to set the ENTRY value,
 * but the ENTRY list is used to set the parameter value and position.
 */
static void assign_value_attribute(ENTRY e, CAL cal, String attribute, String *value, int *pos)
{
	int    n;
	String att = CAL_get_attribute(cal, attribute);

	if (value) *value = NONE;
	if (pos  ) *pos   = 0;

	/* Set item value to the none item as a default */
	for(n = 0; n < e->nitems; n++)
	{
		if(!ISNONE(e->items[n])) continue;
		e->value = e->items[n];
		break;
	}

	/* Find item value in the item list */
	for(n = 0; n < e->nitems; n++)
	{
		if(!same(att, e->items[n])) continue;
		e->value = e->items[n];
		break;
	}

	/* Assign to value the entry list item */
	for(n = 0; n < e->nlist; n++)
	{
		if(e->value != e->list[n]) continue;
		if (pos  ) *pos = n;
		if (value) *value = e->list[n];
		break;
	}
}

/* Find the item list key and create the items list and the corresponding
 * selectable items list. Note that we do not need the number of key items
 * specified. Return the maximum length of the items.
 */
static int create_item_list( ENTRY e)
{
	int      nitems = 0;
	int      n, ml = 1;
	Boolean  have_default;
	String   ptr;
	String   data;
	String   *items = NULL;

	if(!get_key_data(KEY_ITEMS, &data)) return 1;
	
	have_default = (e->default_override && e->override_type == OvTypeValue && e->type != Composite && !e->comp_parent);

	while(!blank(ptr = string_arg(data)) && nitems < NITEMS_LIMIT)
	{
		/* For ranges we have to evaluate default_override. We must convert it to a number
		 * and back as we use a string test and the format must be identical. The only way
		 * to ensure this is to check the number and remake default_override.
		 */
		if(same_start_ic(ptr, "range:"))
		{
			int     dec;
			float   min, max, inc, def = 0;
			char    *p, buf[50];
			Boolean ok, have_dec, ok_default;

			/* Find the default value */
			ok_default = (have_default && !ISNONE(e->default_override) && sscanf(e->default_override,"%f",&def) == 1);
			
			/* The values need to be in a separate buffer due to the *_arg() fcns */
			(void) memset(buf, 0, 50);
			(void) strncpy(buf, ptr+6, 49);

			/* Strip out commas */
			while((p = strchr(buf,','))) *p = ' ';

			/* Parse limits */
			        min = float_arg(buf, &ok);
			if (ok) max = float_arg(buf, &ok);
			if (ok) inc = float_arg(buf, &ok);
			if (ok) dec = int_arg(buf, &have_dec);
			if (ok)
			{
				if(have_dec && dec > 0) /* float */
				{
					float r;
					char fmt[20];

					(void) snprintf(fmt, sizeof(fmt), "%%.%df", dec);
					items = MoreStringArray(items, nitems + (int)((max-min)/inc + .5) + 1);
					for(r = min, n = 0; r < max && n < NITEMS_LIMIT; n++)
					{
						r = min + (float)n * inc;
						items[nitems] = AllocPrint(fmt, r);
						if(ok_default && r == def)
						{
							FreeItem(e->default_override);
							e->default_override = AllocPrint(fmt, r);
						}
						nitems++;
					}
				}
				else /* integer */
				{
					int imin = NINT(min);
					int imax = NINT(max);
					int iinc = NINT(inc);
					int idef = NINT(def);

					if(iinc < 1) iinc = 1;	/* don't want a divide by zero case */

					/* Create the array of values */
					items = MoreStringArray(items, nitems + (imax-imin)/iinc + 1);
					for(n = imin; n <= imax && nitems < NITEMS_LIMIT; n += iinc)
					{
						items[nitems] = AllocPrint("%d", n);
						if(ok_default && n == idef)
						{
							FreeItem(e->default_override);
							e->default_override = AllocPrint("%d", n);
						}
						nitems++;
					}
				}
			}
			else
			{
				pr_error(module, "Error parsing %s in menu file \"%s\"\n", data, menu_file);
			}
		}
		else
		{
			items = MoreStringArray(items, nitems + 1);
			items[nitems] = XtNewString(ISNONE(ptr)? NONE:ptr);
			nitems++;
		}
	}

	if(nitems >= NITEMS_LIMIT)
	{
		pr_error(module, "List of values for object %s exceeded 10,000 in number.\n", e->id);
	}

	/* Set up our item arrays */
	e->nitems = nitems;
	e->items  = items;
	e->nlist  = nitems;
	e->list   = NewStringArray(nitems);
	e->ilabs  = NewStringArray(nitems);
	e->nvis   = key_value(KEY_NVISIBLE,nitems);

	for(n = 0; n < nitems; n++)
	{
		e->list[n] = e->items[n];
		ml = MAX(ml, (int) safe_strlen(items[n]));
	}

	/* Check for a valid default value */
	if(have_default)
	{
		if(ISNONE(e->default_override))
		{
			for( n = 0; n < e->nitems; n++ )
				if(ISNONE(e->items[n])) break;
		}
		else
		{
			for( n = 0; n < e->nitems; n++ )
				if(same(e->default_override, e->items[n])) break;
		}
		if(n >= e->nitems)
		{
			pr_error(module, "Default for object %s is not in list of values\n", e->id);
		}
	}

	/* Check for labels for the items that will override what the user
	 * sees as the item values. If none is found just duplicate the items
	 * array. This array should be the same length as the item array, but
	 * if not use the item itself as a replacement.
	 */
	if(get_key_data(KEY_ITEM_LABELS, &data))
	{
		for( n = 0; n < nitems; n++)
		{
			ptr = string_arg(data);
			if (ptr)
				e->ilabs[n] = XtNewString(ptr);
			else if(ISNONE(e->items[n]))
				e->ilabs[n] = XtNewString(e->none_sub);
			else
				e->ilabs[n] = XtNewString(e->items[n]);
		}
	}
	else
	{
		for( n = 0; n < nitems; n++)
		{
			if(ISNONE(e->items[n]))
				e->ilabs[n] = XtNewString(e->none_sub);
			else
				e->ilabs[n] = XtNewString(e->items[n]);
		}
	}

	/* If there is a clumns override in the data return it, else
	 * return the max number of characters found earlier.
	 */
	return key_value(KEY_NCOLS,ml);
}



/* Redisplay the entry value
 */
static void update_value_display( ENTRY ep )
{
	int    n;
	Widget w, popup;
	String label = value_label(ep);

	switch(ep->type)
	{
		case Popup:
			XtVaGetValues(ep->w, XmNsubMenuId, &popup, NULL);
			w = XtNameToWidget(popup, label);
			XtVaSetValues(ep->w, XmNmenuHistory, w, NULL);
			break;

		case Scroll:
			XuComboBoxSelectItem(ep->w, label, False);
			break;

		case Viewer:
			XmTextFieldSetString(ep->w, label);
			break;
			
		case Spinbox:
			if (sscanf(ep->value, "%d", &n) == 1)
				XtVaSetValues(ep->w, XmNvalue, n, NULL);
			break;	

		case Spinlist:
			XmpSpinBoxSetItem(ep->w, label, False);
			break;

		case Btnlist:
			XuWidgetLabel(ep->w, label);
			break;

		case RadioButtons:
			XuToggleButtonSet(XtNameToWidget(ep->w,label), True, False);
			break;
	}
}


/* Parse the limit data buffer looking for an entry like "GE cloud_top" and
 * fill in the limiting information. As this can be done twice, putting it
 * in a function reduces code.
 */
static Boolean parse_limit( ENTRY e, int ndx )
{
	int i;
	String p = string_arg(e->limit->data);

	if(     same_ic(p,"LT")) e->limit->select[ndx] = LimLT;
	else if(same_ic(p,"LE")) e->limit->select[ndx] = LimLE;
	else if(same_ic(p,"GE")) e->limit->select[ndx] = LimGE;
	else if(same_ic(p,"GT")) e->limit->select[ndx] = LimGT;
	else return False;

	/* The next item will be the limiting attribute. If the entry is
	 * part of a composite we must first check the composite members.
	 */
	p = string_arg(e->limit->data);

	if(e->comp_parent)
	{
		String buf = make_full_id(e, p);
		for(i = 0; i < e->comp_parent->nchild; i++)
		{
			if(!same(buf, e->comp_parent->child_entry[i]->id)) continue;
			e->limit->entry[ndx] = e->comp_parent->child_entry[i];
			break;
		}
		FreeItem(buf);
	}

	if(e->limit->entry[ndx]) return True;

	for( i = 0; i < nentry; i++ )
	{
		if(!same(p, entry[i].id)) continue;
		e->limit->entry[ndx] = &entry[i];
		break;
	}

	if(!e->limit->entry[ndx])
	{
		String id = e->id;
		if(e->comp_parent) id = e->comp_parent->id;
		pr_error(module, "Unrecognized limiting attribute id \"%s\" in object %s\n", p, id);
		return False;
	}
	return True;
}



/* Set the value limits for objects that have been found to have them. This is
 * done after all of the objects have been created as the limit information in
 * the menu file can forward reference and we want all of the objects to exist
 * before processing the limits as we assign pointers into the entry object
 * structure. This makes the logic and processing much easier.
 *
 * The expected input string will have a pattern like
 *
 *      GT cloud_base:cb1 AND GT cloud_base_cb2
 *
 * Note that the part after and including the AND is optional.
 */
static void set_limiting_attributes(void)
{
	int     n;
	String  p, buf;
	Boolean ok;

	for( n = 0; n < nentry; n++ )
	{
		if(!entry[n].limit) continue;

		buf = XtNewString(entry[n].limit->data);
		ok = parse_limit(&entry[n], 0);
		if(ok)
		{
			p = string_arg(entry[n].limit->data);
			if (!p) continue;

			if(     same_ic(p,"AND")) entry[n].limit->logic = LimAND;
			else if(same_ic(p,"OR") ) entry[n].limit->logic = LimOR;
			else
			{
				ok = False;
				pr_error(module, "Unrecognized logical operator \"%s\". AND or OR expected.\n", p);
			}

			if (ok) ok = parse_limit(&entry[n], 1);
		}

		if(!ok)
		{
			pr_error(module, "Error parsing value limiting line \"%s\"\n", buf);
			FreeItem(entry[n].limit);
		}
		FreeItem(buf);
	}
}



/* Create a list limited by the logic in the limiting structure. Note that
 * the list array must have been allocated external to this function.
 */
static int get_limited_list(ENTRY e, int sndx, String *list)
{
	int     i, nlist;
	Boolean ok, found, eq, before;
	String  limit_value;

	/* The numerical comparision assumes that the list contains values
	 * that go in ascending order.
	 */
	nlist = 0;
	limit_value = e->limit->entry[sndx]->value;

	if(e->limit->type == LimTypeString)
	{
		/* The string limit does not do a numberical comparison of strings,
		 * but finds the first entry in the items list that matches the
		 * limit parameter and passes all entries above or below that value.
		 */
		switch(e->limit->select[0])
		{
			case LimLT: before = True;  ok = True;  eq = False; break;
			case LimLE: before = True;  ok = True;  eq = True;  break;
			case LimGE: before = True;  ok = False; eq = True;  break;
			case LimGT: before = False; ok = False; eq = False; break;
		}

		for( i = 0; i < e->nitems; i++)
		{
			found = same(e->items[i], limit_value);
			if(found && before) ok = !ok;
			if(ISNONE(limit_value) || ok || (found && eq))
			{
				list[nlist] = e->items[i];
				nlist++;
			}
			if(found && !before) ok = !ok;
		}
	}
	else if(ISNONE(limit_value))
	{
		for( i = 0; i < e->nitems; i++)
			list[i] = e->items[i];
		nlist = e->nitems;
	}
	else if(e->limit->type == LimTypeInteger)
	{
		int lim;
		found = True;
		if(sscanf(limit_value, "%d", &lim) != 1)
		{
			if(*limit_value == '>') lim = INT_MAX;
			else if(*limit_value == '<') lim = INT_MIN;
			else found = False;
		}
		if (found)
		{
			for( i = 0; i < e->nitems; i++)
			{
				ok = False;
				if(ISNONE(e->items[i]))
				{
					ok = True;
				}
				else
				{
					int val;
					found = True;
					if(sscanf(e->items[i], "%d", &val) != 1)
					{
						if(*e->items[i] == '>') val = INT_MAX;
						else if(*e->items[i] == '<') val = INT_MIN;
						else found = False;
					}
					if (found)
					{
						switch(e->limit->select[sndx])
						{
							case LimLT: ok = (val <  lim); break;
							case LimLE: ok = (val <= lim); break;
							case LimGE: ok = (val >= lim); break;
							case LimGT: ok = (val >  lim); break;
						}
					}
				}
				if(ok)
				{
					list[nlist] = e->items[i];
					nlist++;
				}
			}
		}
	}
	else if(e->limit->type == LimTypeFloat)
	{
		float lim, n, d;
		ok = True;
		if(sscanf(limit_value, "%f/%f", &n, &d) == 2)
		{
			lim = n/d;
		}
		else if(sscanf(limit_value, "%f", &lim) != 1)
		{
			if(*limit_value == '>') lim = FLT_MAX;
			else if(*limit_value == '<') lim = FLT_MIN;
			else ok = False;
		}
		if (ok)
		{
			for( i = 0; i < e->nitems; i++)
			{
				ok = False;
				if(ISNONE(e->items[i]))
				{
					ok = True;
				}
				else
				{
					float val;
					found = True;
					if(sscanf(e->items[i], "%f/%f", &n, &d) == 2)
					{
						val = n/d;
					}
					else if(sscanf(e->items[i], "%f", &val) != 1)
					{
						if(*e->items[i] == '>') val = FLT_MAX;
						else if(*e->items[i] == '<') val = FLT_MIN;
						else found = False;
					}
					if (found)
					{
						switch(e->limit->select[sndx])
						{
							case LimLT: ok = (val <  lim); break;
							case LimLE: ok = (val <= lim); break;
							case LimGE: ok = (val >= lim); break;
							case LimGT: ok = (val >  lim); break;
						}
					}
				}
				if(ok)
				{
					list[nlist] = e->items[i];
					nlist++;
				}
			}
		}
	}
	return nlist;
}


/* Set the available list of values of e according to the values found
 * in the list of limiting objects.
 */
static void set_values_by_limits(ENTRY e)
{
	int      i, n, nchild, nlist1, nlist2;
	Widget   parent, *children;
	String   *labels;
	String   none_entry = NULL;
	String   *list1 = NULL;
	String   *list2 = NULL;

	if (!e->limit) return;

	if( e->limit->logic == LimNone )
	{
		e->nlist = get_limited_list(e, 0, e->list);
		for( i = 0; i < e->nlist; i++ )
			if(ISNONE(e->list[i])) none_entry = e->list[i];
	}
	else
	{
		list1  = NewStringArray(e->nitems);
		list2  = NewStringArray(e->nitems);
		nlist1 = get_limited_list(e, 0, list1);
		nlist2 = get_limited_list(e, 1, list2);

		e->nlist = 0;
		if(e->limit->logic == LimAND)
		{
			for( i = 0; i < nlist1; i++ )
			{
				for( n = 0; n < nlist2; n++ )
				{
					if(list1[i] == list2[n])
					{
						e->list[e->nlist] = list1[i];
						if(ISNONE(e->list[e->nlist])) none_entry = e->list[e->nlist];
						e->nlist++;
					}
				}
			}
		}
		else
		{
			for( i = 0; i < nlist1; i++ )
			{
				for( n = 0; n < e->nlist; n++ )
				{
					if(list1[i] == e->list[n]) break;
				}
				if( n >= e->nlist )
				{
					e->list[e->nlist] = list1[i];
					if(ISNONE(e->list[e->nlist])) none_entry = e->list[e->nlist];
					e->nlist++;
				}
			}
			for( i = 0; i < nlist2; i++ )
			{
				for( n = 0; n < e->nlist; n++ )
				{
					if(list2[i] == e->list[n]) break;
				}
				if( n >= e->nlist )
				{
					e->list[e->nlist] = list2[i];
					if(ISNONE(e->list[e->nlist])) none_entry = e->list[e->nlist];
					e->nlist++;
				}
			}
		}
		FreeItem(list1);
		FreeItem(list2);
	}

	/* Check for a reset of the current value of the entry. If the
	 * current entry is none we will not change it otherwise if it
	 * is not in the new list set it to the first valid entry in 
	 * the list.
	 */
	if(!ISNONE(e->value))
	{
		for( i = 0; i < e->nlist; i++ )
			if(e->value == e->list[i]) break;
		if(i >= e->nlist)
		{
			for( i = 0; i < e->nlist; i++ )
				if(!ISNONE(e->list[i])) break;
			if(i < e->nlist) e->value = e->list[i];
		}
	}

	/* Because of the above, if we have just one list entry and it
	 * is NONE, then the current value will not have been set so we
	 * need to do it here.
	 */
	if(e->nlist == 1 && ISNONE(e->items[0]))
	{
		e->value = e->items[0];
	}
	else if(e->nlist == 0) /* paranoia */
	{
		e->value   = none_entry;
		e->list[0] = none_entry;
		e->nlist   = 1;
	}


	/* Update the displayed object lists
	 */
	labels = list_labels(e);
	switch(e->type)
	{
		case Popup:
			XtVaGetValues(e->w, XmNsubMenuId, &parent, NULL);
			XtVaGetValues(parent, XmNnumChildren, &nchild, XmNchildren, &children, NULL);
			XtUnmanageChildren(children, nchild);
			for( i = 0; i < e->nlist; i++)
			{
				Widget w = XtNameToWidget(parent, labels[i]);
				if (w) XtManageChild(w);
			}
			break;

		case Scroll:
			XuComboBoxDeleteAllItems(e->w);
			XuComboBoxAddItems(e->w, labels, e->nlist, 0);
			XtVaSetValues(e->w, XmNvisibleItemCount, MIN(e->nvis,e->nlist), NULL);
			break;

		case Btnlist:
			XuListLoad(e->w, labels, e->nlist, MIN(e->nvis,e->nlist));
			break;

		case Spinlist:
			XtVaSetValues(e->w, XmNitemCount, e->nlist, XmNitems, labels, NULL);
			XmpSpinBoxSetItem(e->w, value_label(e), False);
			break;
	}

	/* Bugfix 20050624.1 - Update CAL when limit changes the value */
	if(e->comp_parent)
		build_composite_attribute(e->comp_parent);
	else if (!initializing)
		CAL_set_attribute(cal, XtName(e->w), CV(e));

	update_value_display(e);
	update_text_displays(cal);
}



/* Check to see if there are any objects limited by a change in the value
 * of entry object ep.
 */
static void check_for_value_limit(ENTRY ep, const Boolean in_recursion)
{
	int   i;
	static int count;

	if(IsNull(ep) || IsNull(ep->value)) return;

	/* This is just in case we get into an endless loop */
	if (!in_recursion) count = 0;
	if(count++ > 1000)
	{
		pr_error("Entry Menu","More than 1000 iterations checking value limits on \"%s\".\n", ep->id);
		return;
	}

	for(i = 0; i < nentry; i++)
	{
		ENTRY e = entry + i;
		if(!e->limit) continue;
		if(ep == e->limit->entry[0] || ep == e->limit->entry[1])
		{
			/* The object e is limited by ep so do the check */
			set_values_by_limits(e);
			/* Check if any objects are then impacted by a change in e */
			check_for_value_limit(e, True);
		}
	}
}


/* Update the text display windows.
*/
static void update_text_displays(CAL in_cal)
{
	int    i;
	String value;

	if (initializing) return;
	
	/* invoke_rules is a pointer to the actual function called to process rules
	 */
	invoke_rules(in_cal, active_entry_field->info->element->name, active_entry_field->info->level->name);

	value = CAL_get_attribute(in_cal, CALautolabel);
	if(CAL_no_value(value)) value = "";
	if(NotNull(valueDisplay))
	{
		XmTextSetString(valueDisplay, STRVAL(value));
	}
	if(NotNull(labelDisplay) && !lock_label_display)
	{
		String val = CAL_get_attribute(in_cal, CALuserlabel);
		updating_label_display = True;
		if(blank(val))
			XmTextSetString(labelDisplay, STRVAL(value));
		else
			XmTextSetString(labelDisplay, STRVAL(val));
		updating_label_display = False;
	}
	value = CAL_get_attribute(in_cal, CALcategory);
	if(CAL_no_value(value)) value = "";
	if(NotNull(categoryDisplay))
	{
		XmTextFieldSetString(categoryDisplay, STRVAL(value));
	}

	/* Update any entry menu Viewer items */
	for(i = 0; i < nentry; i++)
	{
		if( entry[i].type != Viewer ) continue;
		entry[i].value = CAL_get_attribute(in_cal, XtName(entry[i].w));
		XmTextFieldSetString(entry[i].w, value_label(&entry[i]));
	}
}


/* Set the lists and text objects to the values in the in_cal structure.
*/
static void set_objects_to_cal_values(CAL in_cal, String select_attribute)
{
	int      i, n, pos;
	char     mbuf[200];
	String   attribute, value;
	Boolean  ok;
	Widget   w, popup;
	ENTRY    e;

	if (IsNull(in_cal)) return;

	initializing = True;
	for(i = 0; i < nentry; i++)
	{
		e = &entry[i];
		if(!XtIsSensitive(e->w)) continue;

		/* If we have a composite as a parent then we will be be set by the composite
		 * set procedure and do not do our own.
		 */
		if(e->comp_parent) continue;

		attribute = XtName(e->w);
		if(NotNull(select_attribute) && !same(attribute, select_attribute)) continue;

		switch(e->type)
		{
			case Popup:
				assign_value_attribute(e, in_cal, attribute, &value, NULL);
				XtVaGetValues(e->w, XmNsubMenuId, &popup, NULL);
				w = XtNameToWidget(popup, item_label(e,value));
				if (w) XtVaSetValues(e->w, XmNmenuHistory, w, NULL);
				break;

			case Scroll:
				assign_value_attribute(e, in_cal, attribute, &value, NULL);
				XuComboBoxSelectItem(e->w, item_label(e,value), False);
				break;

			case Text:
				value = CAL_get_attribute(in_cal, attribute);
				XmTextSetString(e->w, STRVAL(value));
				break;

			case Composite:
				(void) set_composite_values(e, CAL_get_attribute(in_cal, attribute));
				break;

			case Viewer:
				e->value = CAL_get_attribute(in_cal, attribute);
				XmTextFieldSetString(e->w, value_label(e));
				break;
				
			case Spinbox:
				(void) safe_strcpy(mbuf, CAL_get_attribute(in_cal, attribute));
				n = int_arg(mbuf, &ok);
				if (ok) XtVaSetValues(e->w, XmNvalue, n, NULL);
				break;	

			case Spinlist:
				assign_value_attribute(e, in_cal, attribute, NULL, &pos);
				XtVaSetValues(e->w, XmNvalue, pos, NULL);
				break;

			case Btnlist:
				assign_value_attribute(e, in_cal, attribute, &value, NULL);
				XuWidgetLabel(e->w, item_label(e,value));
				break;

			case RadioButtons:
				assign_value_attribute(e, in_cal, attribute, &value, NULL);
				XuToggleButtonSet(XtNameToWidget(e->w,item_label(e,value)), True, False);
				break;
		}
	}

	/* Check all entries for limits after we have set them above */
	for(i = 0; i < nentry; i++)
		check_for_value_limit(&entry[i], False);

	/* Bugfix 20050627.1 - reset initializing to occur just before we
	 * update the text displays
	 */
	initializing = False;

	update_text_displays(in_cal);
}



/* The entry manager is the body of the tab and holds all of the objects
*  which are to be controlled by the tab. If there is only one tab we just
*  create a bulletin board parented to the dialog.
*/
static void create_tab_body(void)
{
	XmString xmlabel;

	depth = 1;

	/* this is one case where we do not use block_start as the
	 * parents variable is being initialized
	 */
	fpos = cpos;

	if (!multitabs) return;

	(void) memset((void*)&parents[1], 0, (size_t)(nparents-1)*sizeof(Widget));

	xmlabel = XmStringCreateLocalized(labels[ntabs]);

	parents[1] = XmVaCreateManagedBulletinBoard(tabs, labels[ntabs],
		XmNtabLabelString, xmlabel,
		NULL);

	XmStringFree(xmlabel);
	ntabs++;
}


/* This allows the users to get somewhat fancy and put some of the lists
*  inside of labeled frames.
*/
static void create_frame(void)
{
	int      x = 0, y = 0, width = 200, height = 200;
	Widget   frame;
	XmString label;

	if(!block_start()) return;

	key_values(KEY_POSN, &x, &y);
	key_values(KEY_SIZE, &width, &height);

	frame = XmVaCreateManagedFrame(parents[depth], "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNx,          x,
		XmNy,          y,
		NULL);

	if(label_found(&label))
	{
		(void)XmVaCreateManagedLabel(frame, "frameLabel",
			XmNchildType, XmFRAME_TITLE_CHILD,
			XmNlabelString, label,
			NULL);
		XmStringFree(label);
	}

	depth++;
	parents[depth] = XmVaCreateManagedBulletinBoard(frame, "fbb",
		XmNwidth,     width,
		XmNheight,    height,
		XmNchildType, XmFRAME_WORKAREA_CHILD,
		NULL);

	block_end();
}


/* used by child objects of the composite object
 */
static void build_composite_attribute(ENTRY ep)
{
	if(!initializing)
	{
		int  i;
		size_t buflen = BUFSIZE;
		String label;
		String buf = NewMem(char,buflen);

		(void) strcpy(buf, "");
		for(i = 0; i < ep->nchild; i++)
		{
			if(ISNONE(ep->child_entry[i]->value)) continue;
			label = value_label(ep->child_entry[i]);
			if(safe_strlen(buf)+safe_strlen(ep->child_entry[i]->prefix)+safe_strlen(label)+1 > buflen)
			{
				buflen += safe_strlen(ep->child_entry[i]->prefix)+safe_strlen(label)+1;
				buf = MoreMem(buf, char, buflen);
			}
			(void) safe_strcat(buf, ep->child_entry[i]->prefix);
			(void) safe_strcat(buf, label);
		}
		while(BUFEND(buf) == ' ') BUFEND(buf) = '\0';
		CAL_set_attribute(cal, XtName(ep->w), blank(buf) ? CAL_NO_VALUE : buf);
		FreeItem(buf);
	}
	update_text_displays(cal);
}


/* Determine if the current block has children embedded in it. If we
 * find a "{" this will be the case.
 */
static Boolean have_children(void)
{
	String k;
	cpos = fpos;
	while((k = get_next_key()))
	{
		if( *k == '{' ) return True;
		if( *k == '}' ) break;
	}
	return False;
}



/* A composite list consists of an (optional) label followed by one or
 * more list objects. The output from all of the list objects is combined
 * by the callback to form one attribute value. On initialization of the
 * composite the attribute is parsed to get back out the individual settings.
 * The composite is not managed when it is created but later so that the layout
 * happens after the child objects are initialized.
 */
static void create_composite()
{
	Cardinal ac = 0;
	XmString label;
	Arg      al[14];
	Boolean  attrib_found;
	Widget   w;
	ENTRY    e;
	
	if(!block_start()) return;

	initialize_entry(Composite, &e, &attrib_found);
	find_attribute_label(e, &label);
	set_position( al, &ac );

	if(label)
	{
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False        ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 0            ); ac++;

		w = XmCreateRowColumn(parents[depth], "aligner", al, ac);

		(void)XmVaCreateManagedLabel(w, "label", XmNlabelString, label, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNspacing,      (Dimension) key_value(KEY_SPACING,3) ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found                         ); ac++;
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL                         ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False                                ); ac++;

		set_margins(al, &ac, 0, 0);
		set_border (al, &ac);

		e->w = XmCreateRowColumn(w, e->id, al, ac);

		XmStringFree(label);
		XtManageChild(w);
	}
	else
	{
		XtSetArg(al[ac], XmNspacing,      (Dimension) key_value(KEY_SPACING,3) ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found                         ); ac++;
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL                         ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False                                ); ac++;

		set_margins(al, &ac, 0, 0);
		set_border (al, &ac);

		e->w = XmCreateRowColumn(parents[depth], e->id, al, ac);
	}

	/* If the composite has children then we have to set ourselves as
	 * the current parent for those children.
	 */
	if(have_children())
	{
		depth++;
		parents[depth] = e->w;
	}
	else
	{
		pr_error(module,"Object \"%s\" defined as a composite but it has no children.\n", e->id);
	}
	block_end();
}



/* The procedure for initializing the composite list is rather involved so
*  it is done here in its own routine instead of in the general initialization
*  function for clarity. The parsing of the item input is done from left to right.
*  The sortlistcmp() function is used by the qsort() procedure to sort the list
*  in descending order of string length. We want to check the longest strings in
*  any list for a match before the sorter ones. For example if there is an input
*  string of FOG/TR we want to match an entry of FOG/TR before an entry of FOG.
*/
typedef struct { int ndx; size_t len; } SO;

static int sortlistcmp(const void *a, const void *b)
{
	return((int) (((SO*)b)->len - ((SO*)a)->len));
}


static Boolean set_composite_subobject(ENTRY ep, int ndx, String *value)
{
	int     pos = 0, n = 0;
	String  p  = *value;
	Boolean have_none_entry = False;
	Boolean rtn = False;
	ENTRY   ce  = ep->child_entry[ndx];

	if(!ce->items) return False;

	/* Set position in case there is a problem. If we have a
	 * "none" entry we want this as our default
	 */
	for(pos = 0, n = 0; n < ce->nitems; n++)
	{
		if(!ISNONE(ce->items[n])) continue;
		pos = n;
		have_none_entry = True;
		break;
	}
	ce->value = ce->items[pos];

	/* Nothing left in the value string is a valid condition */
	if(IsNull(p) || blank(p)) return True;

	/* Bugfix: 20070226
	 * A prefix character must be the first character of the string except
	 * for the case where is is specified as a blank. We then take it as
	 * optional as it is possible for something with a blank prefix to be
	 * the first element found.
	 */
	if(*ce->prefix != 0)
	{
		if(*p != *ce->prefix)
		{
			if( *ce->prefix != ' ' ) return False;
		}
		else
		{
			p++;
		}
	}

	/* We could have nothing after a prefix */
	if(blank(p))
	{
		*value = p;
		return True;
	}

	/* How we process depends on several things:
	 * 1. If it is the last object then whatever is left in the value
	 *    buffer must be an exact match.
	 * 2. If the next object has a prefix then the value before the
	 *    prefix must be an exact match.
	 * 3. If neither of the above then we may have several items together
	 *    like RW+ where the RW and the + are held in different objects
	 *    and we must do a length ordered comparison.
	 */
	if(ndx == ep->nchild-1)
	{
		for(n = 0; n < ce->nitems; n++)
		{
			if(same(p, ce->items[n]) || (ISNONE(p) && ISNONE(ce->items[n])))
			{
				pos = n;
				rtn  = True;
				break;
			}
		}
	}
	else if(*ep->child_entry[ndx+1]->prefix != 0)
	{
		char *e;
		char *buf = NewMem(char, safe_strlen(p)+1);

		e = strchr(p, (int)ep->child_entry[ndx+1]->prefix[0]);
		if(!e) e = p + safe_strlen(p);
		(void) strncpy(buf, p, (size_t)(e - p));

		for(n = 0; n < ce->nitems; n++)
		{
			if(same(buf, ce->items[n]) || (ISNONE(buf) && ISNONE(ce->items[n])))
			{
				pos = n;
				p    = e;
				rtn  = True;
				break;
			}
		}
		FreeItem(buf);
	}
	else
	{
		size_t len = (size_t) safe_strlen(p);
		SO *sortlist = NewMem(SO, ce->nitems);
		for(n = 0; n < ce->nitems; n++)
		{
			sortlist[n].ndx = n;
			sortlist[n].len = (size_t) safe_strlen(ce->items[n]);
		}
		qsort((void*)sortlist, (size_t)ce->nitems, sizeof(SO), sortlistcmp);
		for(n = 0; n < ce->nitems; n++)
		{
			if(len >= sortlist[n].len && strncmp(p, ce->items[sortlist[n].ndx], sortlist[n].len) == 0)
			{
				pos = sortlist[n].ndx;
				p   += sortlist[n].len;
				rtn  = True;
				break;
			}
		}
		FreeItem(sortlist);
	}
	/* Bugfix 20070226
	 * If no match and one of the valid list items is none, then we need 
	 * to return true as cd->value has been set to none above.
	 */
	if (rtn)
		ce->value = ce->items[pos];
	else if (have_none_entry)
		rtn = True;

	*value = p;
	return rtn;
}


/* Parse the input item string into the sub-object values and set the
 * sub-object active value to the appropriate part. If there is an error
 * in the item string set the composite to NONE.
 */
static Boolean set_composite_values(ENTRY ep, String item)
{
	int     i;
	Boolean ok = True;
	String  p = item;

	for(i = 0; i < ep->nchild; i++)
	{
		if(!set_composite_subobject(ep, i, &p)) ok = False;
		update_value_display(ep->child_entry[i]);
	}
	if(!ok && !ISNONE(item))
	{
		pr_error(module,"Value \"%s\" for composite object \"%s\" is invalid.\n", item, ep->id);
		(void) set_composite_values(ep, NONE);
	}
	return ok;
}



/* The following set of functions handles the "button list" object. The btnlist
 * structure is used as only one list is visible at any one time.
 */
static BtnList btnlist = { 0, 0, 0 };


#if (XmVERSION < 2)

static void create_btnlist(void)
{
	if(block_start())
	{
		pr_warning(module, "The button_list object is not supported by this system.\n");
		block_end();
	}
}

#else
/*
 * The following two event handlers are used to coordinate
 * grabs between the scrollbar and the grab shell in dropdown lists.
 * In this case the existing active grab started by the grab shell
 * will interfere with the passive grab started by X when the user
 * presses a button within the scrollbar.  
 *
 * To deal with the problem, sb_btn_down_eh will do an XtGrabPointer
 * to transfer the grab to the scrollbar and sb_btn_up_eh will cause
 * the grab to return to the grab shell.
 */
/*ARGSUSED*/
static void btnlist_list_cb(Widget w , XtPointer client_data , XtPointer call_data)
{
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	btnlist.entry->value = btnlist.entry->list[rtn->item_position - 1];
	XtPopdown(btnlist.popup);

	if(btnlist.entry->comp_parent)
		build_composite_attribute(btnlist.entry->comp_parent);
	else if (!initializing)
		CAL_set_attribute(cal, XtName(btnlist.entry->w), CV(btnlist.entry));

	check_for_value_limit(btnlist.entry, False);
	update_value_display(btnlist.entry);
	update_text_displays(cal);
}



/*ARGSUSED*/
static void btnlist_cb(Widget w , XtPointer client_data , XtPointer call_data)
{
	Position         x, y;
	XtWidgetGeometry size;

	btnlist.entry = (ENTRY)client_data;

	XuListLoad(btnlist.list, list_labels(btnlist.entry), btnlist.entry->nlist,
			MIN(btnlist.entry->nlist,btnlist.entry->nvis));

	/* Position dialog just under the button */
	XtTranslateCoords(w, 0, 0, &x, &y);
	size.request_mode = CWHeight;
	(void) XtQueryGeometry(w, NULL, &size);
	XtVaSetValues(btnlist.popup, XmNx, x-1, XmNy, y+size.height, NULL);
	XtPopupSpringLoaded(btnlist.popup);
}


/* We do not want the popup shell to popdown when button up events happen in the shell
 * so we intercept them here and discard them.
 */
/*ARGSUSED*/
static void btnlist_popup_eh(Widget w, XtPointer client_data, XEvent *event, Boolean *dispatch)
{
	if( event->type == ButtonRelease ) *dispatch = False;
}


static void create_btnlist(void)
{
	int        ncols;
	Cardinal   ac = 0;
	char       buf[256];
	Boolean    attrib_found = True;
	XmString   label;
	Dimension  width, mw, sw;
	ENTRY      e;
	Arg        al[12];
	XmFontList fontlist;

	
	if(!block_start()) return;

	initialize_entry(Btnlist, &e, &attrib_found);
	find_value_limit(e);
	find_attribute_label(e, &label);
	ncols = create_item_list(e);

	XtSetArg(al[ac], XmNsensitive, attrib_found ); ac++;

	if(!key_boolean(KEY_BUTTON_LOOK, True))
	{
		XtSetArg(al[ac], XmNshadowThickness, 0); ac++;
	}

	set_position (al, &ac);
	set_colours  (al, &ac);
	set_margins  (al, &ac, -1, -1);
	set_border   (al, &ac);
	set_alignment(al, &ac);

	e->w = XmCreatePushButton(parents[depth], e->id, al, ac);

	XtAddCallback(e->w, XmNactivateCallback, btnlist_cb, (XtPointer)e);

	/* We fake setting the number of columns by filling a string with '8'
	 * and then using this to set the width. We do this after creating the
	 * button so we can query it for its fontList.
	 */
	XtVaGetValues(e->w, XmNfontList, &fontlist, XmNmarginWidth, &mw, XmNshadowThickness, &sw, NULL);
	(void) memset((void*)buf, '8', MAX(256,(size_t)ncols));
	buf[MAX(255,ncols)] = '\0';
	label = XmStringCreateLocalized(buf);
	width = XmStringWidth(fontlist, label) + 2*mw + 2*sw;
	XmStringFree(label);
	XtVaSetValues(e->w, XmNwidth, width, XmNrecomputeSize, False, NULL);
	XtManageChild(e->w);

	/* create the scrolled list popup. We only use one of these and recycle */
	if (!btnlist.popup)
	{
		ac = 0;
		XtSetArg(al[ac], XmNallowShellResize, True        ); ac++;
		XtSetArg(al[ac], XmNownerEvents,      True        ); ac++;
		XtSetArg(al[ac], XmNgrabStyle,        GrabModeSync); ac++;
		XtSetArg(al[ac], XmNshadowThickness,  1           ); ac++;

		btnlist.popup = XmCreateGrabShell(dialog, "btnlist_popup", al, ac);
		XtAddEventHandler(btnlist.popup, ButtonReleaseMask, FALSE, btnlist_popup_eh, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNselectionPolicy,  XmBROWSE_SELECT     ); ac++;
		XtSetArg(al[ac], XmNlistSizePolicy,   XmRESIZE_IF_POSSIBLE); ac++;
		XtSetArg(al[ac], XmNspacing,          0                   ); ac++;
		XtSetArg(al[ac], XmNlistMarginWidth,  5                   ); ac++;
		XtSetArg(al[ac], XmNlistMarginHeight, 5                   ); ac++;

		btnlist.list = XmCreateScrolledList(btnlist.popup, "listLabelList", al, ac);
		XtManageChild(btnlist.list);
		XtAddCallback(btnlist.list, XmNbrowseSelectionCallback, btnlist_list_cb, NULL);
	}
	
	block_end();
}
#endif


/*ARGSUSED*/
static void popup_list_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int   n;
	ENTRY ep = (ENTRY)client_data;

	if(IsNull(ep)) return;

	if(InList(XtName(w), ep->nitems, ep->ilabs, &n))
	{
		ep->value = ep->items[n];
		if(ep->comp_parent)
			build_composite_attribute(ep->comp_parent);
		else if (!initializing)
			CAL_set_attribute(cal, XtName(ep->w), CV(ep));
	}

	check_for_value_limit(ep, False);
	update_text_displays(cal);
}


/* Create an option list from the data in the menu file.
*/
static void create_popup_list(void)
{
	int        i, x = 0, y = 0;
	XmString   label = NULL;
	Boolean    attrib_found;
	ENTRY      e;
	
	if(!block_start()) return;

	initialize_entry(Popup, &e, &attrib_found);
	find_value_limit(e);
	find_attribute_label(e, &label);
	(void) create_item_list(e);
	key_values(KEY_POSN, &x, &y);

	e->w = XuVaMenuBuildOption(parents[depth], e->id, NULL,
			XmNx,           x,
			XmNy,           y,
			XmNlabelString, label,
			XmNsensitive,   attrib_found,
			XmNmarginWidth, 0,
			XmNmarginHeight,0,
			NULL);

	for(i = 0; i < e->nitems; i++)
	{
		String il = item_label(e, e->items[i]);
		(void)XuMenuAddButton(e->w, il, il, NoId, popup_list_cb, (XtPointer)e);
	}

	if(label) XmStringFree(label);

	block_end();
}



/* Function to parse the program run string, substitute key strings with their
 * values and put them into a form for use by the XuRunProgram function. This
 * requires the arguments to be passed in as a null terminated string array.
 */
/*ARGSUSED*/
static void run_pgm_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int    n = 0;
	String p, pgm, error_msg, line;
	String lat, lon;
	String *info, *list = NULL;

	info = (String *)client_data;
	pgm  = info[0];
	error_msg = info[1];
	line = XtNewString(info[2]);

	lat = CAL_get_attribute(cal, CALlatitude);
	if(!CAL_is_value(lat)) lat = error_msg;

	lon = CAL_get_attribute(cal, CALlongitude);
	if(!CAL_is_value(lon)) lon = error_msg;

	while((p = string_arg(line)))
	{
		String ap = XtNewString(p);
		/* 
		 * Assume that there could be more than one keyword in the token.
		 */
		for(;;)
		{
			       p = ReplaceKeyword(ap, "<SETUP>",     NULL, GetSetupFile(0,NULL));
			if(!p) p = ReplaceKeyword(ap, "<setup>",     NULL, GetSetupFile(0,NULL));
			if(!p) p = ReplaceKeyword(ap, "<RTIME>",     NULL, GV_T0_depict);
			if(!p) p = ReplaceKeyword(ap, "<rtime>",     NULL, GV_T0_depict);
			if(!p) p = ReplaceKeyword(ap, "<VTIME>",     NULL, ActiveDepictionTime(FIELD_DEPENDENT));
			if(!p) p = ReplaceKeyword(ap, "<vtime>",     NULL, ActiveDepictionTime(FIELD_DEPENDENT));
			if(!p) p = ReplaceKeyword(ap, "<ELEMENT>",   NULL, GV_active_field->info->element->name);
			if(!p) p = ReplaceKeyword(ap, "<element>",   NULL, GV_active_field->info->element->name);
			if(!p) p = ReplaceKeyword(ap, "<LEVEL>",     NULL, GV_active_field->info->level->name);
			if(!p) p = ReplaceKeyword(ap, "<level>",     NULL, GV_active_field->info->level->name);
			if(!p) p = ReplaceKeyword(ap, "<LATITUDE>",  NULL, lat);
			if(!p) p = ReplaceKeyword(ap, "<latitude>",  NULL, lat);
			if(!p) p = ReplaceKeyword(ap, "<LONGITUDE>", NULL, lon);
			if(!p) p = ReplaceKeyword(ap, "<longitude>", NULL, lon);
			if(!p) p = ReplaceKeyword(ap, "<ATTRIB:>",   cal,  error_msg);
			if(!p) p = ReplaceKeyword(ap, "<attrib:>",   cal,  error_msg);
			if(!p) break;
			FreeItem(ap);
			ap = p;
		}
		list = MoreStringArray(list, n+1);
		list[n] = ap;
		n++;
	}
	list = MoreStringArray(list, n+1);
	list[n] = NULL;

	(void) XuRunProgram(pgm, list);

	FreeItem(line);
	FreeList(list, n);
}


/*ARGSUSED*/
static void run_pgm_cleanup_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	String *list = (String *)client_data;
	FreeList(list, 3);
}


/* Create a button that will launch a program with parameters as defined in the
 * menu configuration file.
 */
static void create_run_pgm_btn( void )
{
	int      width = 0, height = 0;
	Cardinal ac = 0;
	String   p, *list;
	String   data;
	Widget   btn;
	XmString xmlabel = NULL;
	Arg      al[14];
	
	if(!block_start()) return;

	key_values(KEY_SIZE, &width, &height);
	if(width  > 0) XtSetArg(al[ac], XmNwidth, width);
	else           XtSetArg(al[ac], XmNmarginWidth, 8);
	ac++;
	if(height > 0) XtSetArg(al[ac], XmNheight, height);
	else           XtSetArg(al[ac], XmNmarginHeight, 4);
	ac++;

	if(label_found(&xmlabel))
	{
		XtSetArg(al[ac], XmNlabelString, xmlabel); ac++;
	}

	set_position (al, &ac);
	set_colours  (al, &ac);
	set_margins  (al, &ac, -1, -1);
	set_border   (al, &ac);
	set_alignment(al, &ac);

	btn = XmCreatePushButton(parents[depth], "runPgmBtn", al, ac);

	if (xmlabel) XmStringFree(xmlabel);

	if(get_key_data(KEY_PROGRAM, &data))
	{
		if ((p = string_arg(data)))
		{
			/* list[0] contains the program name
			 * list[1] contains the string to pass if an attribute is missing.
			 * list[2] contains the parameter string from the menu config file.
			 */
			list = NewStringArray(3);
			list[0] = XtNewString(p);
			if(get_key_data(KEY_MISSING, &data)) {
				p = string_arg(data);
				list[1] = XtNewString((p)? p:"none");
			} else {
				list[1] = XtNewString("none");
			}
			if(get_key_data(KEY_PARMS, &data)) {
				list[2] = XtNewString(data);
			}
			XtAddCallback(btn, XmNactivateCallback, run_pgm_cb,        (XtPointer)list);
			XtAddCallback(btn, XmNdestroyCallback,  run_pgm_cleanup_cb, (XtPointer)list);
			XtManageChild(btn);
		}
	}
	else
	{
		pr_error(module, "Run program button has no program defined - button not created.\n");
	}

	XtManageChild(btn);
	block_end();
}


/*ARGSUSED*/
static void radio_buttons_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	ENTRY     ep;
	XtPointer value;

	if(!XmToggleButtonGetState(w)) return;

	XtVaGetValues(XtParent(w), XmNuserData, &value, NULL);
	if(!value) return;
	ep = (ENTRY)value;

	ep->value = ep->items[PTR2INT(client_data)];

	if(ep->comp_parent)
		build_composite_attribute(ep->comp_parent);
	else if (!initializing)
		CAL_set_attribute(cal, XtName(w), CV(ep));

	update_text_displays(cal);
}


static void create_radio_buttons(void)
{
	int      i;
	Cardinal ac = 0;
	XmString label;
	Widget   frame;
	Boolean  attrib_found;
	ENTRY    e;
	Arg      al[12];
	
	if(!block_start()) return;

	initialize_entry(RadioButtons, &e, &attrib_found);
	find_value_limit(e);
	find_attribute_label(e, &label);
	set_position(al, &ac);
	(void) create_item_list(e);

	if(label)
	{
		XtSetArg(al[ac], XmNshadowType,   XmSHADOW_ETCHED_IN); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found      ); ac++;
		XtSetArg(al[ac], XmNmarginWidth,  7                 ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 7                 ); ac++;

		frame = XmCreateFrame(parents[depth], "frame", al, ac);

		(void)XmVaCreateManagedLabel(frame, "frameLabel",
			XmNchildType, XmFRAME_TITLE_CHILD,
			XmNlabelString, label,
			NULL);

		ac = 0;
		XtSetArg(al[ac], XmNuserData,       (XtPointer) e); ac++;
		XtSetArg(al[ac], XmNradioBehavior,  True         ); ac++;

		e->w = XmCreateRowColumn(frame, "rc", al, ac);

		for( i = 0; i < e->nitems; i++ )
		{
			Widget btn = XmVaCreateManagedToggleButton(e->w, e->ilabs[i], NULL);
			XtAddCallback(btn, XmNvalueChangedCallback, radio_buttons_cb, INT2PTR(i));
		}

		XmStringFree(label);
		XtManageChild(frame);
	}
	else
	{
		XtSetArg(al[ac], XmNsensitive,     attrib_found  ); ac++;
		XtSetArg(al[ac], XmNradioBehavior, True          ); ac++;
		XtSetArg(al[ac], XmNuserData,      (XtPointer) e ); ac++;

		e->w = XmCreateRowColumn(parents[depth], "rc", al, ac);

		for( i = 0; i < e->nitems; i++ )
		{
			Widget btn = XmVaCreateManagedToggleButton(e->w, e->ilabs[i], NULL);
			XtAddCallback(btn, XmNvalueChangedCallback, radio_buttons_cb, INT2PTR(i));
		}
	}
	XtManageChild(e->w);
	block_end();
}


/*ARGSUSED*/
static void scrolled_list_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int   ndx;
	ENTRY ep = (ENTRY)client_data;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(IsNull(ep)) return;

	ndx = rtn->item_position - 1;
	if( ndx < 0 ) return;

	ep->value = ep->list[ndx];

	if(ep->comp_parent)
		build_composite_attribute(ep->comp_parent);
	else if (!initializing)
		CAL_set_attribute(cal, XtName(w), CV(ep));

	check_for_value_limit(ep, False);
	update_text_displays(cal);
}


/* Create a ComboBox list from the data in the menu file.
*/
static void create_scrolled_list(void)
{
	int      i, ncols;
	Cardinal ac = 0;
	XmString label;
	Widget   w;
	XmString *xm_array;
	Boolean  attrib_found;
	ENTRY    e;
	Arg      al[12];
	
	if(!block_start()) return;

	initialize_entry(Scroll, &e, &attrib_found);
	find_value_limit(e);
	find_attribute_label(e, &label);
	set_position(al, &ac);
	ncols = create_item_list(e);

	/* the item list is needed in XmString format */
	xm_array = NewMem(XmString, e->nitems);
	for(i=0; i<e->nitems; i++)
		xm_array[i] = XmStringCreateLocalized(e->ilabs[i]);

	if(label)
	{
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False        ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 0            ); ac++;

		w = XmCreateRowColumn(parents[depth], "aligner", al, ac);

		(void)XmVaCreateManagedLabel(w, "label", XmNlabelString, label, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNcolumns,          (short) ncols         ); ac++;
		XtSetArg(al[ac], XmNitemCount,        e->nitems             ); ac++;
		XtSetArg(al[ac], XmNitems,            xm_array              ); ac++;
		XtSetArg(al[ac], XmNvisibleItemCount, MIN(e->nvis,e->nitems)); ac++;
		XtSetArg(al[ac], XmNcomboBoxType,     XmDROP_DOWN_LIST      ); ac++;

		e->w = XmCreateComboBox(w, e->id, al, ac);

		XmStringFree(label);
		XtManageChild(e->w);
		XtManageChild(w);
	}
	else
	{
		XtSetArg(al[ac], XmNsensitive,        attrib_found          ); ac++;
		XtSetArg(al[ac], XmNcolumns,          (short) ncols         ); ac++;
		XtSetArg(al[ac], XmNitemCount,        e->nitems             ); ac++;
		XtSetArg(al[ac], XmNitems,            xm_array              ); ac++;
		XtSetArg(al[ac], XmNvisibleItemCount, MIN(e->nvis,e->nitems)); ac++;
		XtSetArg(al[ac], XmNcomboBoxType,     XmDROP_DOWN_LIST      ); ac++;

		e->w = XmCreateComboBox(parents[depth], e->id, al, ac);
		XtManageChild(e->w);
	}
	XtAddCallback(e->w, XmNselectionCallback, scrolled_list_cb, (XtPointer)e);
	XmStringArrayFree(xm_array, e->nitems);
	block_end();
}


/* We do not want to allow non-printing characters to be entered into the
*  text entry. This includes CR's as well as any others.
*/
/*ARGSUSED*/
static void text_verify_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int len;
	XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call_data;

	if(cbs->text->length == 0) return;   /* A backspace will do this! */

	for(len = 0; len < cbs->text->length; len++)
	{
		if(!isprint((int)cbs->text->ptr[len]))
		{
			int i;
			for( i = len; (i+1) < cbs->text->length; i++)
				cbs->text->ptr[i] = cbs->text->ptr[i+1];
			cbs->text->length--;
			len--;
		}
	}
	if(cbs->text->length <= 0) cbs->doit = False;
}


/*ARGSUSED*/
static void text_line_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	String str;
	ENTRY  ep = (ENTRY)client_data;

	str = XmTextGetString(w);
	if(!same(str,ep->value))
	{
		FreeItem(ep->value);
		ep->value = str;
		if (!initializing) CAL_set_attribute(cal, XtName(w), CV(ep));
		update_text_displays(cal);
	}
	else
	{
		FreeItem(str);
	}
}


/* The following is used for free-form text input. There is no checking
*  done on the contents.
*/
static void create_text(void)
{
	Cardinal ac = 0;
	String   value;
	XmString label;
	Boolean  attrib_found;
	Widget   rc;
	ENTRY    e;
	Arg      al[12];
	
	if(!block_start()) return;

	initialize_entry(Text, &e, &attrib_found);
	find_attribute_label(e, &label);
	set_position(al, &ac);

	value = CAL_get_attribute(cal,e->id);
	if(CAL_no_value(value)) value = "";

	if(label)
	{
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False        ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 0            ); ac++;

		rc = XmCreateRowColumn(parents[depth], "rc", al, ac);

		(void)XmVaCreateManagedLabel(rc, "lab", XmNlabelString, label, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNsensitive, attrib_found                    ); ac++;
		XtSetArg(al[ac], XmNcolumns,   (short) key_value(KEY_NCOLS, 10)); ac++;
		XtSetArg(al[ac], XmNrows,      (short) key_value(KEY_NROWS, 1) ); ac++;
		XtSetArg(al[ac], XmNvalue,     value                           ); ac++;
		XtSetArg(al[ac], XmNwordWrap,  True                            ); ac++;
		XtSetArg(al[ac], XmNeditMode,  XmMULTI_LINE_EDIT               ); ac++;

		e->w = XmCreateText(rc, e->id, al, ac);

		XmStringFree(label);
		XtManageChild(e->w);
		XtManageChild(rc);
	}
	else
	{
		XtSetArg(al[ac], XmNsensitive, attrib_found                    ); ac++;
		XtSetArg(al[ac], XmNcolumns,   (short) key_value(KEY_NCOLS, 10)); ac++;
		XtSetArg(al[ac], XmNrows,      (short) key_value(KEY_NROWS, 1) ); ac++;
		XtSetArg(al[ac], XmNvalue,     value                           ); ac++;
		XtSetArg(al[ac], XmNwordWrap,  True                            ); ac++;
		XtSetArg(al[ac], XmNeditMode,  XmMULTI_LINE_EDIT               ); ac++;

		e->w = XmCreateText(parents[depth], e->id, al, ac);
		XtManageChild(e->w);
	}

	XtAddCallback(e->w, XmNmodifyVerifyCallback, text_verify_cb, NULL);
	XtAddCallback(e->w, XmNlosingFocusCallback, text_line_cb, (XtPointer)e);
	block_end();
}

/* This is the length of the allocated memory associated with the e->value
 * variable. It must be long enough to hold a potential long int value.
 */
#define VALUE_LEN	24

/*ARGSUSED*/
static void spinbox_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	ENTRY  ep = (ENTRY)client_data;
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;

	if(rtn->reason != XmCR_OK) return;

	(void) snprintf(ep->value, VALUE_LEN, "%d", rtn->value);

	if(ep->comp_parent)
		build_composite_attribute(ep->comp_parent);
	else if (!initializing)
		CAL_set_attribute(cal, XtName(w), CV(ep));

	check_for_value_limit(ep, False);
	update_text_displays(cal);
}


static void create_spinbox(void)
{
	int      n;
	int      valmax, valmin, valset = 50;
	short    ncols;
	long     increment;
	Cardinal ac = 0;
	char     value[128];
	XmString label;
	Boolean  ok, attrib_found;
	Widget   rc;
	ENTRY    e;
	Arg      al[10];
	
	if(!block_start()) return;

	initialize_entry(Spinbox, &e, &attrib_found);
	find_attribute_label(e, &label);
	set_position(al, &ac);

	valmax = key_value(KEY_VALUE_MAX, 100);
	valmin = key_value(KEY_VALUE_MIN, 0);
	if(valset < valmin || valset > valmax)
		valset = valmin + (valmax - valmin)/2;
	increment = (long) key_value(KEY_INCREMENT,1);
	ncols     = (short) key_value(KEY_NCOLS,8);
	
	e->value = NewMem(char,VALUE_LEN);

	(void) safe_strcpy(value, "");
	if(e->default_override)
	{
		if(e->override_type == OvTypeValue)
			(void) safe_strcpy(value, e->default_override);
		else
			(void) safe_strcpy(value, CAL_get_attribute(cal, e->default_override));
	}
	else
	{
		String val = CAL_get_attribute(cal, e->id);
		if(!CAL_no_value(val)) (void) safe_strcpy(value, val);
	}

	if(!blank(value))
	{
		n = int_arg(value, &ok);
		if (ok) valset = n;
	}

	/* Make sure that there is a default value for the spinbox */
	FreeItem(e->default_override);
	e->default_override = AllocPrint("%d", valset);

	if(label)
	{
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False        ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 0            ); ac++;

		rc = XmCreateRowColumn(parents[depth], "rc", al, ac);

		(void)XmVaCreateManagedLabel(rc, "lab", XmNlabelString, label, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNspinBoxType, XmSPINBOX_NUMBER); ac++;
		XtSetArg(al[ac], XmNeditable,    False           ); ac++;
		XtSetArg(al[ac], XmNcolumns,     ncols           ); ac++; 
		XtSetArg(al[ac], XmNmaximum,     valmax          ); ac++;
		XtSetArg(al[ac], XmNminimum,     valmin          ); ac++;
		XtSetArg(al[ac], XmNincrement,   increment       ); ac++;
		XtSetArg(al[ac], XmNvalue,       valset          ); ac++;

		e->w = XmpCreateSpinBox(rc, e->id, al, ac);
		
		XmStringFree(label);
		XtManageChild(e->w);
		XtManageChild(rc);
	}
	else
	{
		XtSetArg(al[ac], XmNspinBoxType, XmSPINBOX_NUMBER); ac++;
		XtSetArg(al[ac], XmNeditable,    False           ); ac++;
		XtSetArg(al[ac], XmNsensitive,   attrib_found    ); ac++;
		XtSetArg(al[ac], XmNcolumns,     ncols           ); ac++; 
		XtSetArg(al[ac], XmNmaximum,     valmax          ); ac++;
		XtSetArg(al[ac], XmNminimum,     valmin          ); ac++;
		XtSetArg(al[ac], XmNincrement,   increment       ); ac++;
		XtSetArg(al[ac], XmNvalue,       valset          ); ac++;

		e->w = XmpCreateSpinBox(parents[depth], e->id, al, ac);
		XtManageChild(e->w);
	}
	XtAddCallback(e->w, XmNvalueChangedCallback, spinbox_cb, (XtPointer)e);
	block_end();
}


/*ARGSUSED*/
static void spin_list_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	ENTRY ep = (ENTRY)client_data;
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;

	if(IsNull(ep)) return;

	if(rtn->reason != XmCR_OK) return;

	ep->value = ep->list[rtn->value];

	if(ep->comp_parent)
		build_composite_attribute(ep->comp_parent);
	else if (!initializing)
		CAL_set_attribute(cal, XtName(w), CV(ep));

	check_for_value_limit(ep, False);
	update_text_displays(cal);
}


static void create_spin_list(void)
{
	int      ncols;
	Cardinal ac = 0;
	XmString label;
	Widget   w;
	Boolean  attrib_found;
	ENTRY    e;
	Arg      al[14];
	
	if(!block_start()) return;

	initialize_entry(Spinlist, &e, &attrib_found);
	find_value_limit(e);
	find_attribute_label(e, &label);
	set_position(al, &ac);
	ncols = create_item_list(e);

	if(label)
	{
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False        ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 0            ); ac++;

		w = XmCreateRowColumn(parents[depth], "aligner", al, ac);

		(void)XmVaCreateManagedLabel(w, "label", XmNlabelString, label, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNspinBoxType,     XmSPINBOX_STRINGS); ac++;
		XtSetArg(al[ac], XmNeditable,        False            ); ac++;
		XtSetArg(al[ac], XmNpositionType,    XmPOSITION_INDEX ); ac++;
		XtSetArg(al[ac], XmNcolumns,         (short) ncols    ); ac++;
		XtSetArg(al[ac], XmNitemCount,       e->nitems        ); ac++;
		XtSetArg(al[ac], XmNitems,           e->ilabs         ); ac++;
		XtSetArg(al[ac], XmNwrap,            key_boolean(KEY_WRAP, False)); ac++;

		e->w = XmpCreateSpinBox(w, e->id, al, ac);

		XtVaSetValues(XmpSpinBoxGetTextField(e->w), XmNmarginHeight, 4, XmNmarginWidth, 4, NULL);
		XmStringFree(label);
		XtManageChild(e->w);
		XtManageChild(w);
	}
	else
	{
		XtSetArg(al[ac], XmNspinBoxType,     XmSPINBOX_STRINGS); ac++;
		XtSetArg(al[ac], XmNsensitive,       attrib_found     ); ac++;
		XtSetArg(al[ac], XmNeditable,        False            ); ac++;
		XtSetArg(al[ac], XmNpositionType,    XmPOSITION_INDEX ); ac++;
		XtSetArg(al[ac], XmNcolumns,         (short) ncols    ); ac++;
		XtSetArg(al[ac], XmNitemCount,       e->nitems        ); ac++;
		XtSetArg(al[ac], XmNitems,           e->ilabs         ); ac++;
		XtSetArg(al[ac], XmNwrap,            key_boolean(KEY_WRAP, False)); ac++;

		e->w = XmpCreateSpinBox(parents[depth], e->id, al, ac);
		XtVaSetValues(XmpSpinBoxGetTextField(e->w), XmNmarginHeight, 4, XmNmarginWidth, 4, NULL);
		XtManageChild(e->w);

	}
	XtAddCallback(e->w, XmNvalueChangedCallback, spin_list_cb, (XtPointer)e);
	block_end();
}


/* This simply allows the user to place a label anywhere on the current
*  parent. The label does not have to relate to anything in particular.
*/
static void create_display_label(void)
{
	Cardinal ac = 0;
	XmString label = NULL;
	Arg      al[10];
	Widget   w;
	
	if(!block_start()) return;

	if(label_found(&label))
	{
		XtSetArg(al[ac], XmNlabelString, label); ac++;
	}

	set_position(al, &ac);
	set_colours (al, &ac);
	set_margins (al, &ac, -1, -1);
	set_border  (al, &ac);

	w = XmCreateLabel(parents[depth], "label", al, ac);
	XtManageChild(w);

	if (label) XmStringFree(label);

	block_end();
}


/* This simply allows the user to place a line anywhere on the current
*  parent.
*/
static void create_separator(void)
{
	Cardinal  ac = 0;
	Dimension length;
	String    types[6];
	String    data;
	Arg       al[8];
	Widget    w;

	static int xmt[] = { XmSHADOW_ETCHED_IN,   XmSHADOW_ETCHED_OUT,XmSINGLE_DASHED_LINE,
		               	 XmDOUBLE_DASHED_LINE, XmSINGLE_LINE,      XmDOUBLE_LINE};
	types[0] = KEY_ETCHED_IN;
	types[1] = KEY_ETCHED_OUT;
	types[2] = KEY_SINGLE_DASH;
	types[3] = KEY_DOUBLE_DASH;
	types[4] = KEY_SINGLE;
	types[5] = KEY_DOUBLE;
	
	if(!block_start()) return;

	length = (Dimension) key_value(KEY_LENGTH,50); 
	if(get_key_data(KEY_ORIENTATION, &data))
	{
		if(same_start_ic(data,"v"))
		{
			XtSetArg(al[ac], XmNorientation, XmVERTICAL); ac++;
			XtSetArg(al[ac], XmNheight, length); ac++;
		}
		else
		{
			XtSetArg(al[ac], XmNwidth, length); ac++;
		}
	}
	else
	{
		XtSetArg(al[ac], XmNwidth, length); ac++;
	}

	if(get_key_data(KEY_LINE_STYLE, &data))
	{
		int n;
		String ptr = string_arg(data);
		for(n = 0; n > (int) XtNumber(types); n++)
		{
			if(!same_ic(ptr,types[n])) continue;
			XtSetArg(al[ac], XmNseparatorType, xmt[n]); ac++;
			break;
		}
	}

	set_position(al, &ac);
	set_colours (al, &ac);

	w = XmCreateSeparator(parents[depth], "label", al, ac);
	XtManageChild(w);

	block_end();
}


/* Set the attributes named in the client_data to the default cal state
*  as given in menu file and reinitialize the display objects.
*/
/*ARGSUSED*/
static void clear_btn_activate_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	int    i;
	String p, list, val;

	list = XtNewString((String)client_data);

	while((p = string_arg(list)))
	{
		val = NULL;
		for( i = 0; i < nentry; i++ )
		{
			if(same(p, entry[i].id) && NotNull(entry[i].default_override))
			{
				if( entry[i].override_type == OvTypeValue )
					val = entry[i].default_override;
				else
					val = CAL_get_attribute(cal, entry[i].default_override);
				break;
			}
		}
		if (val) CAL_set_attribute(cal, p, ISNONE(val)? CAL_NO_VALUE:val);
		set_objects_to_cal_values(cal, p);
	}
	FreeItem(list);
}


/*ARGSUSED*/
static void clear_btn_destroy_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	FreeItem(client_data);
}


static void create_clear_btn(void)
{
	Cardinal ac = 0;
	size_t   buflen = 0;
	String   buf = NULL;
	String   data = NULL;
	String   p = NULL;
	XmString label = NULL;
	Widget   btn;
	Arg      al[12];
	
	if(!block_start()) return;

	set_position(al, &ac);
	set_colours (al, &ac);
	set_margins (al, &ac, -1, -1);
	set_border  (al, &ac);

	if(label_found(&label))
	{
		XtSetArg(al[ac], XmNlabelString, label); ac++;
	}
	btn = XmCreatePushButton(parents[depth], "clearBtn", al, ac);
	XtManageChild(btn);
	if (label) XmStringFree(label);

	/* Determine how much memory we need to assemble the list */
	if(get_key_data(KEY_ATTRIB_ID_LIST, &data))
	{
		while((p = string_arg(data)))
		{
			if(CAL_has_attribute(cal, p))
				buflen += safe_strlen(p)+1;
			else
				pr_error(module, "Unrecognized attribute \"%s\" in menu file.\n", p);
		}
	}

	/* Allocate and build the list */
	buf = NewMem(char,buflen+2);
	(void) strcpy(buf,"");
	if(get_key_data(KEY_ATTRIB_ID_LIST, &data))
	{
		while((p = string_arg(data)))
		{
			if(CAL_has_attribute(cal, p))
			{
				(void) safe_strcat(buf, p);
				(void) safe_strcat(buf, " ");
			}
		}
	}

	if(blank(buf))
	{
		pr_error(module, "Clear button has no valid list of attributes to clear.\n");
		FreeItem(buf);
	}
	else
	{
		BUFEND(buf) = '\0';
		XtAddCallback(btn, XmNactivateCallback, clear_btn_activate_cb, (XtPointer) buf);
		XtAddCallback(btn, XmNdestroyCallback,  clear_btn_destroy_cb,  (XtPointer) buf);
	}

	block_end();
}


/*ARGSUSED*/
static void set_last_drawn_cb(Widget w , XtPointer client_data , XtPointer call_data )
{

	/* Set the user label to whatever is in the display.
	*/
	if(NotNull(labelDisplay))
	{
		String  str = XmTextGetString(labelDisplay);
		CAL_set_attribute(cal, CALuserlabel, str);
		FreeItem(str);
	}
	if(InEditMode(E_MODIFY))
		(void) IngredEditCommand("EDIT MODIFY SET", cal, NullCal);
	else if(InEditMode(E_DIVIDE))
		(void) IngredEditCommand("EDIT DIVIDE SET", cal, NullCal);
	else
		(void) IngredEditCommand("EDIT DRAW SET", cal, NullCal);
}


/*ARGSUSED*/
static void enter_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	MAKE_ENTRY_FCN action_fcn = (MAKE_ENTRY_FCN)client_data;

	/* Set the user label to whatever is in the display.
	*/
	if(NotNull(labelDisplay))
	{
		String  str = XmTextGetString(labelDisplay);
		CAL_set_attribute(cal, CALuserlabel, str);
		FreeItem(str);
	}

	if (action_fcn)
		action_fcn(cal);
}


/*ARGSUSED*/
static void enter_and_exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	enter_cb(w, client_data, call_data);
	exit_cb(NULL, EXIT_NORMAL, NULL);
}


/* Create a display only object from the data in the menu file.
*/
static void create_viewer(void)
{
	Cardinal ac = 0;
	Boolean  attrib_found;
	XmString label;
	Pixel    bkgnd;
	Widget   w;
	ENTRY    e;
	Arg      al[12];
	
	if(!block_start()) return;

	initialize_entry(Viewer, &e, &attrib_found);
	find_attribute_label(e, &label);
	set_position(al, &ac);

	/* We want those items which are not allowed to be changed to be visually
	 * different from those that can be changed. This is done by setting the
	 * background colour of the non-editable text fields to the background
	 * colour of their parent.
	 */
	XtVaGetValues(parents[depth], XmNbackground, &bkgnd, NULL);
	
	if(label)
	{
		XtSetArg(al[ac], XmNorientation,  XmHORIZONTAL  ); ac++;
		XtSetArg(al[ac], XmNisAligned,    False         ); ac++;
		XtSetArg(al[ac], XmNsensitive,    attrib_found  ); ac++;
		XtSetArg(al[ac], XmNbackground,   bkgnd         ); ac++;
		XtSetArg(al[ac], XmNmarginHeight, 0             ); ac++;

		w = XmCreateRowColumn(parents[depth], "rc", al, ac);
		(void)XmVaCreateManagedLabel(w, "label", XmNlabelString, label, NULL);

		ac = 0;
		XtSetArg(al[ac], XmNbackground,            bkgnd ); ac++;
		XtSetArg(al[ac], XmNcursorPositionVisible, False ); ac++;
		XtSetArg(al[ac], XmNeditable,              False ); ac++;
		XtSetArg(al[ac], XmNcolumns,               (short) key_value(KEY_NCOLS,10)); ac++;

		e->w = XmCreateTextField(w, e->id, al, ac);

		XmStringFree(label);
		XtManageChild(e->w);
		XtManageChild(w);
	}
	else
	{
		XtSetArg(al[ac], XmNsensitive,             attrib_found); ac++;
		XtSetArg(al[ac], XmNbackground,            bkgnd       ); ac++;
		XtSetArg(al[ac], XmNcursorPositionVisible, False       ); ac++;
		XtSetArg(al[ac], XmNeditable,              False       ); ac++;
		XtSetArg(al[ac], XmNcolumns,               (short) key_value(KEY_NCOLS,10)); ac++;

		e->w = XmCreateTextField(parents[depth], e->id, al, ac);
		XtManageChild(e->w);
	}
	block_end();
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i;

	if(IsNull(dialog)) return;

	XtUnmapWidget(XuGetShell(dialog));
	XuUpdateDisplay(dialog);
	XuDestroyDialog(dialog);

	/* If an exit is the result of a cancel button activation sometimes
	 * we need to tell ingred something to do to remove whatever action
	 * was done. This especially applies to labels.
	 *
	 * XXX Not possible yet - for future consideration XXX
	 *
	 */
	if( client_data != EXIT_NORMAL )
	{
	}

	for(i = 0; i < nentry; i++)
	{
		if(entry[i].type == Text   ) FreeItem(entry[i].value);
		if(entry[i].type == Spinbox) FreeItem(entry[i].value);

		FreeItem(entry[i].id);
		FreeItem(entry[i].limit);
		FreeItem(entry[i].default_override);
		FreeItem(entry[i].none_sub);
		FreeItem(entry[i].child_entry);
		FreeItem(entry[i].list);
		FreeList(entry[i].items, entry[i].nitems);
		FreeList(entry[i].ilabs, entry[i].nitems);
	}
	nentry = 0;

	FreeItem(parents);
	FreeItem(entry);
	FreeItem(tabPages);

	if(btnlist.popup)
	{
		XtRemoveEventHandler(btnlist.popup, ButtonReleaseMask, FALSE, btnlist_popup_eh, NULL);
		XtDestroyWidget(btnlist.popup);
		btnlist.popup = NULL;
	}

	cal   = CAL_destroy(cal);
	incal = CAL_destroy(incal);

	lock_label_display     = False;
	updating_label_display = False;

	setLastDrawnBtn    = NullWidget;
	active_entry_field = (FIELD_INFO*)NULL;
	type_active        = TYPE_NONE;
	dialog             = NullWidget;
	menu_file          = NULL;
}
