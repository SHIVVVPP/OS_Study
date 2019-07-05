// ordered_array.h --	interface for creating, inserting and deleting
//						from ordered arrays.
//						

#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include "windef.h"

// ordered_array는 삽입시 정렬한다.( 항상 정렬된 상태를 유지한다. )
// 어떤 타입이든 상관 없이 정렬할 수 있도록 type_t 를 void*로 한다.
typedef void* type_t;

// 첫 번째 파라미터가 두 번째 파라미터보다 작을 경우, predicate는 0이 아닌 값으로 반환해야 한다. 그렇지 않으면 0을 반환한다.
typedef s8int(*lessthan_predicate_t)(type_t, type_t);

typedef struct
{
	type_t* array;
	u32int size;
	u32int max_size;
	lessthan_predicate_t less_than;
}ordered_array_t;

// 비교 함수
s8int standard_lessthan_predicate(type_t a, type_t b);

// ordered_array 생성함수
ordered_array_t create_ordered_array(u32int max_size, lessthan_predicate_t less_than);
ordered_array_t place_ordered_array(void* addr, u32int max_size, lessthan_predicate_t less_than);

void destroy_ordered_array(ordered_array_t* array);

void insert_ordered_array(type_t item, ordered_array_t* array);


type_t lookup_ordered_array(u32int i, ordered_array_t* array);

void remove_ordered_array(u32int i, ordered_array_t* array);


#endif