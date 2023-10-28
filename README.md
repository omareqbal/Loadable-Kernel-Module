# Loadable Kernel Module

This project implements Loadable Kernel Modules with handling for concurrency and separate data from multiple processes. Following are the two Loadble Kernel Modules -

- Module 1 - This provides sorting of an array of N values (string/integer) within the kernel space. The user process writes the values using the module and then reads the sorted array.

- Module 2 - This builds a binary search tree using the provided values (string/integer). The user process writes the values and sets the order as preorder, inorder or postorder. It then reads the values using the module in the specified order. It can also search a value in the tree and get information about the nodes of the tree.
