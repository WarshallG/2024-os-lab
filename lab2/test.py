
import time

dma_test = DMA(10)
# 请求序列
workload = [
    {'type': 'malloc', 'id': 1, 'size': 3, 'value': ['a', 'b', 'c']},
    {'type': 'malloc', 'id': 2, 'size': 2, 'value': ['#', '#']},
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
        print(dma_test.frag)
    else:
        print(dma_test.free(request['id']))
        print(dma_test.frag)

time2 = time.time()
print(f"time is : {time2 - time1}")

# offset = 0
# new_heap = ['#'] * len(self.heap)
# new_allocated_blocks = {}
# # for id, (start, size) in sorted(self.allocated_blocks.items(), key=lambda x: x[1][0]):
# for id, (start, size) in self.allocated_blocks.items():
#     new_allocated_blocks[id] = (offset, size)
#     new_heap[offset:offset + size] = self.heap[start:start + size]
#     offset += size
#
# self.heap = new_heap
# self.allocated_blocks = new_allocated_blocks
# self.free_blocks = [(offset, len(self.heap) - offset)]