import heapq


class Allocation:
    def __init__(self, id, start, size, value):
        self.id = id
        self.start = start
        self.size = size
        self.value = value

    def __lt__(self, other):
        return self.start < other.start  # 根据起始位置比较


class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.size = size
        self.allocations = []  # 用于跟踪分配的内存块，优先队列

    def malloc(self, id: int, size: int, value: list) -> bool:
        free_index = self.find_free_space(size)
        if free_index is None:
            self.compact()
            free_index = self.find_free_space(size)
            if free_index is None:
                return False  # 没有足够的空间分配

        self.heap[free_index:free_index + size] = value
        #
        # print(self.heap)
        alloc_obj = Allocation(id, free_index, size, value)
        heapq.heappush(self.allocations, alloc_obj)  # 将 Allocation 对象插入堆中
        return True

    def free(self, id: int) -> bool:
        for i, allocation in enumerate(self.allocations):
            if allocation.id == id:
                del self.allocations[i]
                start = allocation.start
                size = allocation.size
                self.heap[start:start + size] = ['#'] * size
                #
                # print(self.heap)
                heapq.heapify(self.allocations)  # 重新堆化

                # 执行碎片整合
                # self.defragment(start)
                return True
        return False  # 没有找到对应的分配

    def data(self) -> dict:
        heap_data = {}
        for allocation in self.allocations:
            # 获取分配的信息
            allocation_id = allocation.id
            start = allocation.start
            size = allocation.size
            value = allocation.value

            # 构建字典
            heap_data[allocation_id] = {'start': start, 'size': size, 'value': value}

        return heap_data

    # 辅助函数：寻找空闲空间
    def find_free_space(self, size):
        for i in range(self.size - size + 1):
            if all(c == '#' for c in self.heap[i:i + size]):
                return i
        # for i, allocation in enumerate(self.allocations):
        #     if i == 0:
        #         if allocation.start >= size:
        #             return 0
        #     else:
        #         if allocation.start - pre_allocation_end >= size:
        #             return pre_allocation_end
        #     pre_allocation_end = allocation.start + allocation.size
        # num_allocations = len(self.allocations)
        # if num_allocations > 0:
        #     last_allocation_end = self.allocations[num_allocations - 1].start + self.allocations[num_allocations - 1].size
        # else:
        #     last_allocation_end = 0
        # if self.size - last_allocation_end >= size:
        #     return last_allocation_end
        return None

    def compact(self) -> None:
        # 移动已分配空间
        count = 0
        for allocation in self.allocations:
            start = allocation.start
            size = allocation.size
            value = allocation.value
            if count == start:
                count += size
            else:
                allocation.start = count
                self.heap[count:count + size] = value
                count += size
        self.heap[count:] = ['#'] * (self.size - count)

        print(self.heap)


import time

dma_test = DMA(10)
# 请求序列
workload = [
    {'type': 'malloc', 'id': 1, 'size': 3, 'value': ['a', 'b', 'c']},
    {'type': 'malloc', 'id': 2, 'size': 2, 'value': ['t', 'u']},
    {'type': 'malloc', 'id': 3, 'size': 4, 'value': ['1', '2', '3', '4']},
    {'type': 'free', 'id': 2},
    {'type': 'malloc', 'id': 4, 'size': 3, 'value': ['5', '6', '7']},
    {'type': 'malloc', 'id': 5, 'size': 3, 'value': ['5', '6', '7']},
    {'type': 'free', 'id': 1}
]

time1 = time.time()
for request in workload:
    if request['type'] == 'malloc':
        print(dma_test.malloc(request['id'], request['size'], request['value']))
    else:
        print(dma_test.free(request['id']))

time2 = time.time()
print(f"time is : {time2 - time1}")