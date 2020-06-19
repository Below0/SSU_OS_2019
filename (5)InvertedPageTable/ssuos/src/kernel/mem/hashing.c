#include <device/io.h>
#include <mem/mm.h>
#include <mem/paging.h>
#include <device/console.h>
#include <proc/proc.h>
#include <interrupt.h>
#include <mem/palloc.h>
#include <ssulib.h>
#include <mem/hashing.h>


uint32_t F_IDX(uint32_t addr, uint32_t capacity) { // 첫번째 해쉬함수
    return addr % ((capacity / 2) - 1);
}

uint32_t S_IDX(uint32_t addr, uint32_t capacity) { // 두번째 해쉬함수
    return (addr * 7) % ((capacity / 2) - 1) + capacity / 2;
}

/* 아이템 이동 시 카피하기 위한 함수*/
int copy_hash(uint32_t hash_idx, entry* copy_entry, int height){
  level_bucket* bucket; // 버켓 포인터
  if(height == 1) bucket = &hash_table.top_buckets[hash_idx]; // 높이 1일 경우 top_bucket을 참조
  else bucket = &hash_table.bottom_buckets[hash_idx]; // 아닐 경우 bottom 참조

  for(int i = 0; i < SLOT_NUM; i++){ // 해당 버켓의 모든 슬롯을 탐색
    if(bucket->token[i] == 0){ // 비어있는 경우 키와 벨류를 복사
      printk("copy key = %d, value = %x ",copy_entry->key, copy_entry->value);
      printk("into bucket[%d][%d]\n",hash_idx,i);
      bucket->slot[i].key = copy_entry->key;
      bucket->slot[i].value = copy_entry->value;
      bucket->token[i] = 1;
      return 1; // 복사되었을 경우 1 리턴
      }
    }
  return -1; // 복사 안되었을 경우 -1 리턴
}
/* 아이템 이동 함수 */
int move_hash(uint32_t* hash_idx, uint32_t cvalue, uint32_t ckey){
  int i,j, idx;
  uint32_t VH_addr;
  uint32_t move_idx[2]; // 이동할 수 있는 두 인덱스 값

  for(i = 0; i < 2; i++){
    for(j = 0; j < SLOT_NUM; j++){
      VH_addr = RH_TO_VH(hash_table.top_buckets[hash_idx[i]].slot[j].value);
	  /* 해당 아이템의 value(실제 주소)를 가상 주소로 변환하여 저장*/
      move_idx[0] = F_IDX(VH_addr, CAPACITY); // 가상함수를 통해 두 해쉬 함수 수행
      move_idx[1] = S_IDX(VH_addr, CAPACITY);
      if(move_idx[0] == hash_idx[i]) idx = 1; // 현재 위치가 아닌 인덱스로 이동함
      else idx = 0;
      if(copy_hash(move_idx[idx], &hash_table.top_buckets[hash_idx[i]].slot[j],1) == 1){ // 해당 아이템이 이동 성공할 경우
        printk("move and hash value inserted in top level : idx = %d, key = %d, value = %x\n",hash_idx[i],ckey,cvalue);
        hash_table.top_buckets[hash_idx[i]].token[j] = 1; // 현재 위치에 삽입함
        hash_table.top_buckets[hash_idx[i]].slot[j].key = ckey;
        hash_table.top_buckets[hash_idx[i]].slot[j].value = cvalue;
      return 1; // 성공 시 1 리턴
    }
    }
  }
  /* top_bucket에 이동시킬 아이템 없는 경우 */
  for(i = 0; i < 2; i++){
    for(j = 0; j < SLOT_NUM; j++){ // top 탐색과 같은 방식으로 수행
      VH_addr = RH_TO_VH(hash_table.bottom_buckets[hash_idx[i]].slot[j].value);
      move_idx[0] = F_IDX(VH_addr, CAPACITY)/2;
      move_idx[1] = S_IDX(VH_addr, CAPACITY)/2;
      if(move_idx[0] == hash_idx[i]/2) idx = 1;
      else idx = 0;
      if(copy_hash(move_idx[idx], &hash_table.bottom_buckets[hash_idx[i]].slot[j],1) == 1){
        printk("move and hash value inserted in bottom level : idx = %d, key = %d, value = %x\n",hash_idx[i]/2,ckey,cvalue);
        hash_table.bottom_buckets[hash_idx[i]].token[j] = 1;
        hash_table.bottom_buckets[hash_idx[i]].slot[j].key = ckey;
        hash_table.bottom_buckets[hash_idx[i]].slot[j].value = cvalue;
      return 1;
      }

    }
  }

return -1; // 이동 후 삽입 실패 시 -1 리턴
}

int insert_hash(size_t page_idx, uint32_t VH_addr){ // 아이템 삽입 함수
  uint32_t RH_addr = VH_TO_RH(VH_addr);
  uint32_t hash_idx[2]; //해쉬함수를 통하여 넣을 수 있는 두 인덱스 계산
  hash_idx[0] = F_IDX(VH_addr, CAPACITY);  hash_idx[1] = S_IDX(VH_addr, CAPACITY);
/* top level */
    for(int j = 0; j < SLOT_NUM; j++){
        for(int i = 0; i < 2; i++){
if(hash_table.top_buckets[hash_idx[i]].token[j] == 0){ // 해당 entry가 비어있는 경우 삽입
  printk("hash value inserted in top level : idx = %d, key = %d, value = %x\n",hash_idx[i],page_idx,RH_addr);
    hash_table.top_buckets[hash_idx[i]].slot[j].key = page_idx;
    hash_table.top_buckets[hash_idx[i]].slot[j].value = RH_addr;
    hash_table.top_buckets[hash_idx[i]].token[j] = 1; // 아이템 삽입 됨을 표시
    return 1; // 성공 시 1 리턴
  }
}
/* bottom level */
uint32_t bottom_idx;
  for(int j = 0; j < SLOT_NUM; j++){
    for(int i = 0; i < 2; i++){
        bottom_idx = hash_idx[i]/2; // bottom의 인덱스로 변환
    if(hash_table.bottom_buckets[bottom_idx].token[j] == 0){ // bottom의 해당 entry 비어있는 경우 삽입
        printk("hash value inserted in bottom level : idx = %d, key = %d, value = %x\n",bottom_idx,page_idx,RH_addr);
      hash_table.bottom_buckets[bottom_idx].slot[j].key = page_idx;
      hash_table.bottom_buckets[bottom_idx].slot[j].value = RH_addr;
      hash_table.bottom_buckets[bottom_idx].token[j] = 1;
      return 1; // 성공 시 1 리턴
    }
  }
}
  }
  move_hash(hash_idx,RH_addr,page_idx); // 모든 entry 꽉 차있는 경우 한 아이템을 이동시킬 수 있는지 확인
  return 0;
}
int delete_hash(size_t page_idx, uint32_t VH_addr){ // 삭제 함수
	uint32_t RH_addr = VH_TO_RH(VH_addr);
  uint32_t hash_idx[2]; // 해당 아이템이 들어갈 수 있는 두 인덱스 계산
  hash_idx[0] = F_IDX(VH_addr, CAPACITY);
  hash_idx[1] = S_IDX(VH_addr, CAPACITY);
  for(int i = 0; i < 2; i++){
    for(int j = 0; j < SLOT_NUM; j++){
  if(hash_table.top_buckets[hash_idx[i]].token[j] == 1){ // 해당 슬롯이 차 있는 경우 확인
    if(hash_table.top_buckets[hash_idx[i]].slot[j].key == page_idx){ //key를 통해 일치 여부 확인
    hash_table.top_buckets[hash_idx[i]].token[j] = 0; // 일치하는 경우 해당 슬롯을 비움(=0)
    printk("hash value deleted : idx : %d, key : %d, value : %x\n",hash_idx[i],page_idx, hash_table.top_buckets[hash_idx[i]].slot[j].value);
    return 1;//삭제 성공시 1 리턴
  }
  }
  }
  uint32_t bottom_idx; // top에 없을 경우 bottom 탐색
  for(int i = 0; i < 2; i++){
  bottom_idx = hash_idx[i]/2;//bottom의 인덱스로 변환
  for(int j = 0; j < SLOT_NUM; j++){
    if(hash_table.bottom_buckets[bottom_idx].token[j] == 1){ // 위와 동일한 방식
      if(hash_table.bottom_buckets[hash_idx[i]].slot[j].key == page_idx){
      hash_table.bottom_buckets[hash_idx[i]].token[j] = 0;
      printk("hash value deleted : idx : %d, key : %d, value : %x\n",bottom_idx,page_idx,hash_table.bottom_buckets[bottom_idx].slot[j].value);
      return 1;
    }
    }
  }
  }

  }

}

void init_hash_table(void){ // 해쉬 테이블 초기화
  int i , j;
  /* top, bottom bucket의 모든 token을 0으로 초기화 */
  for(i = 0; i < CAPACITY; i++){
    for(j = 0; j < SLOT_NUM; j++)
    hash_table.top_buckets[i].token[j] = 0;
  }
for(i = 0 ; i < CAPACITY/2; i++){
  for(j = 0; j < SLOT_NUM; j++)
  hash_table.bottom_buckets[i].token[j] = 0;
}
	// TODO : OS_P5 assignment
}
