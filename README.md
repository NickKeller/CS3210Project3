CS3210Project3

This is the assignment description given to me by my professor at Georgia Tech.

I was responsible for writing all of the FUSE code, found in pfs.c and pfs.h. I also wrote the perl program that runs the filesystem: runProgram.pl

==============
Replicated Photo Store

How often is it that we create backups of our data in different locations or drives on our machines ? It is also quite often that we forget where we made the backup and many times we forget to synchronise them after making a change to one of the copies. Big companies like Google, Amazon, Facebook, etc. store petabytes of data in their datacenters and managing this data is a huge problem. What if one of the storage devices crashes ? What if the data gets corrupted ? Several mechanisms to solve this problem have been developed like Dynamo (http://dl.acm.org/citation.cfm?id=1294281) by Amazon. Recently, cloud storage provided by various vendors like Dropbox, Google drive, etc. are being more and more used to offload the data storage.

In this project, you are going to build a fault tolerant, replicated photo store implementing some of the techniques proposed in the Dynamo paper. You will be using the FUSE file system to build a userspace replicated file system. You will be making different mount points to emulate different drives or nodes in you system and a master drive from where you read and write the photos.

RPFS Features

This assignment is reasonably open-ended, and there is room for considerable innovation on your part.  The minimum requirements are listed below, but feel free to be creative and go above and beyond.

1)  Automatic Replication and retrieval -  Pictures should only be copied into and displayed from the root level of one of the file systems which we call as the master node. The replicas (or nodes) are to be used for fault tolerance purposes internally by RPFS. RPFS should automatically move/link the files to the appropriate nodes. For example, suppose you create 10 mount points for RPFS and ~/MyPFS in your home directory is your master node, then the user can read and write pictures to ~/MyPFS only and the other mount points serve as replica nodes. For retrieval, RPFS should check for any active nodes and maintain high availability of the system if any of the node fails. It should not show those files all of whose replicas have failed.

2)  Hash function -  You have to implement consistent hashing for RPFS. The choice of hash function is left as a design choice for you. You can use existing hash implementations or develop one of your own. You can decide to hash on the file name, file type, file contents or any other other combination of file attributes. Each hash implementation will have tradeoffs like performance, number of collisions, load distribution, etc.

3)  Distributed Back-end Storage - You won't be implementing a full, block-level file system, so any files in ~/MyPFS will have to have a representation in a back-end store. Conceptually, this could be anything -- a database, a magic file in /tmp, a hidden ~/.mypics directory. However, as we are motivated by thinking about cloud systems, I want you to come up with a decomposition between storing files on the local disk and a remote server (on a home file server, flickr, pinterest, etc.).         

You will need some representation of the metadata on the local hardware for fast lookups, but how you manage display and storage between local and remote locations has to be part of your design and performance measurement for this project.

Further ideas:
Feel free to develop this scenario and go as far as your team wants! What happens when you make a copy of a file - do you create replicas for the copy or just link it to the existing replicas ? What if you delete one of the copies ? How do you deal with non-uniform data and load distribution - can you use the concept of “virtual nodes” ? How do you synchronise the replicas if a crashed node becomes active again ? We look forward to seeing where you end up!

Tools

You will be using the FUSE toolkit to implement your file system for this project.  Accessing complex libraries from the kernel is problematic, and FUSE helps to solve this.  FUSE, in essence, brings a bit of microkernel to the Linux macrokernel.  It implements the Linux virtual file system interface in the kernel by providing callback functions to a registered userspace program.  The userspace daemon can then perform the action as requested and supply the updates to the inodes, directory structures, etc. through a set of provided functions.

You can go and download FUSE and build and install it for your kernel if you like.  However, a package for the redhat kernel, for ubuntu, and for Android already exist.  There are a number of tutorials and "hello world" examples to read online about how to get started with fuse.  Useful pages to start with are
http://fuse.sourceforge.net/  -- The main page for all things fuse.
http://fuse.sourceforge.net/helloworld.html  -- a simple hello world\
 
Report

You will need to turn in the source code of your implementation of RPFS along with a report on detailed design of the file systems, the assumptions made during the design process, implications for reliability, measurement of its performance and how the design scales with the number of images and future enhancement potential. As before, please put all of your source files into a single tarball (.tgz file) with the report as a separate PDF file. Graphs of scalability and performance should be included. (Do not utilize anything that you do not want displayed publicly in class with your entire family watching as well....)

Presentations will be done in class, during the final week of classes (i.e. Dead Week). You will have ~8 minutes to do a presentation (with slides) describing your design choices, your user scenario, and to demonstrate your code running on the system. It is recommended that you demonstrate the file system live -- running it on a local laptop, exporting nautilus from one of the factors, etc. Remember to think of this as if you are pitching your code base to a bunch of venture capital folks -- fun, informative, and technically brilliant are what we’re looking for. (no pressure, of course).
