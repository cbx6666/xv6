// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock lock;
  struct buf head;
} bucket[NBUCKET];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

// 哈希函数：将 (dev, blockno) 映射到桶
int
hash(uint dev, uint blockno)
{
  return (dev + blockno) % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // 初始化每个桶的锁和链表
  for(int i = 0; i < NBUCKET; i++){
    char name[16];
    snprintf(name, sizeof(name), "bcache.bucket_%d", i);
    initlock(&bucket[i].lock, name);
    
    // 初始化桶的循环链表
    bucket[i].head.prev = &bucket[i].head;
    bucket[i].head.next = &bucket[i].head;
  }

  // 将所有缓存块分散到各个桶中
  for(b = bcache.buf; b < bcache.buf + NBUF; b++){
    int bucket_id = (b - bcache.buf) % NBUCKET;

    b->next = bucket[bucket_id].head.next;
    b->prev = &bucket[bucket_id].head;
    initsleeplock(&b->lock, "buffer");
    b->timestamp = ticks;
    bucket[bucket_id].head.next->prev = b;
    bucket[bucket_id].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket_id = hash(dev, blockno);

  acquire(&bucket[bucket_id].lock);

  // 在对应桶中查找是否已缓存
  for(b = bucket[bucket_id].head.next; b != &bucket[bucket_id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket[bucket_id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 缓存未命中，需要分配新的缓存块
  release(&bucket[bucket_id].lock);

  acquire(&bcache.lock);
  acquire(&bucket[bucket_id].lock);

  // 双重检查：可能在等待锁期间其他进程已经缓存了该块
  for(b = bucket[bucket_id].head.next; b != &bucket[bucket_id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket[bucket_id].lock);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  // 查找可重用的缓存块（LRU策略）
  struct buf *least_recent = 0;
  uint least_recent_ticks = 0xFFFFFFFF;

  // 先在当前桶中查找
  for(b = bucket[bucket_id].head.next; b != &bucket[bucket_id].head; b = b->next){
    if(b->refcnt == 0 && b->timestamp < least_recent_ticks){
      least_recent = b;
      least_recent_ticks = b->timestamp;
    }
  }

  // 如果当前桶没有可用的，查找其他桶
  if(!least_recent){
    for(int i = 0; i < NBUCKET; i++){
      if(i == bucket_id) 
        continue;
      
      acquire(&bucket[i].lock);
      for(b = bucket[i].head.next; b != &bucket[i].head; b = b->next){
        if(b->refcnt == 0 && b->timestamp < least_recent_ticks){
          least_recent = b;
          least_recent_ticks = b->timestamp;
        }
      }
      release(&bucket[i].lock);
    }
  }

  if(!least_recent) {
    panic("bget: no buffers");
  }

  // 如果选中的缓存块在其他桶中，需要移动它
  if(least_recent){
    int old_bucket = hash(least_recent->dev, least_recent->blockno);
    if(old_bucket != bucket_id){
      acquire(&bucket[old_bucket].lock);
      // 从旧桶中移除
      least_recent->next->prev = least_recent->prev;
      least_recent->prev->next = least_recent->next;
      release(&bucket[old_bucket].lock);
      
      // 添加到新桶
      least_recent->next = bucket[bucket_id].head.next;
      least_recent->prev = &bucket[bucket_id].head;
      bucket[bucket_id].head.next->prev = least_recent;
      bucket[bucket_id].head.next = least_recent;
    }
    least_recent->dev = dev;
    least_recent->blockno = blockno;
    least_recent->valid = 0;
    least_recent->refcnt = 1;

    release(&bucket[bucket_id].lock);
    release(&bcache.lock);
    acquiresleep(&least_recent->lock);
    return least_recent;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket_id = hash(b->dev, b->blockno);

  acquire(&bucket[bucket_id].lock);
  b->refcnt--;
  if(b->refcnt == 0){
    b->timestamp = ticks;
  }
  release(&bucket[bucket_id].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


