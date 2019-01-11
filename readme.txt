File: readme.txt
Author: Minju Kim
----------------------

implicit.c
--------
For my implicit allocator, I utilized a "best fit" algorithm to search for free blocks. To implement this, I created a struct that would hold two parameters: a ptr to the payload of a block and the size of said block (as indicated by its header). I then iterated through the heap, comparing the "best fit block" and the "current block" to see which free block provided a closer fit to the requestedsz, and updated "best fit block" accordingly. I thought using a "best fit" algorithm would maximize the utilization of implicit, because it does not utilize other efficiency-boosting features such as coalescing or in-place realloc.
Overall, this allocator is decently good at utilizing free space because it will always return a ptr to the block that is closest to the size requested by the client's use of mymalloc. However, it does not utilize coalescing or in-place realloc or splitting of any kind, so if there are consecutive free blocks on the heap, the allocator will not recognize those as one large free block. This is an example of internal fragmentation and can decrease the efficiency of the heap. Since implicit also does not have a free list, as does explicit, the allocator must often traverse through the entirety of the "used" heap to find a free block. This can cause a delay in performance time, which is most evident when running the larger files such as "trace-".


explicit.c
--------
For explicit, the biggest design choice I made was to implement the LIFO method, or "last in first out" method. I mainly chose this method because I thought it was a good compromise between a speedy addition of blocks to the free list and a decently efficient search method for finding free blocks. I did not implement footers because I planned to only coalesce to the right; I thought nailing this step and putting more time into making sure my coalescing algorithm was correct was more important than trying to implement coalescing on both left and right. My explicit allocator achieves a greater utilization with much less instructions per request than my implicit allocator. Its weakness lies in its inability to split recently allocated blocks; it only splits blocks when realloc is called. This leaves room for some utilization inefficiencies.
