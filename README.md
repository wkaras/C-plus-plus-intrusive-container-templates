# C-plus-plus-intrusive-container-templates
C++ intrusive container templates.  Abstract node links, no use of
new/delete (AVL tree, singly-linked list, bidirection list, hash table
available currently).

Also look at boost::instrusive, which is STL-compatible.  Links under the
Boost approach are unabstracted pointers.  There is no function to build
a balanced binary tree from a sequence, but you could port mine into the
Boost lib pretty easily.

There are no throw statements in this code.  But if you use abstractor
classes that throw exceptions, you must assume the data structure is in an
undefined state after an exception is thrown.  boost::instrusive, as well
as the STL, will restore the data structure to the pre-call state in the
event of an exception throw.
