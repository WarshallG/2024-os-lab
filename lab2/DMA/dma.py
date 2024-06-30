class DMA:
    def __init__(self, size):
        self.heap = ['#'] * size
        self.free_blocks = [(0, size)]  # 空闲块的列表: (start, length)
        self.allocated_blocks = {}      # 已分配块的列表: {id: (start, length, value)}
        self.size = size                # 内存总大小
        self.allocated_size = 0         # 已经分配了的总内存的大小
        self.frag = 1                   # 外部碎片的个数，最多不会超过self.size / 2 个
        self.ratio = 0.00055            # 以此为指标决定是否自主整理内存

    def malloc(self, id: int, size: int, value: list) -> bool:
        # 如果带分配内存的大小大于内存的剩余空间，一定不能分配
        if size > self.size - self.allocated_size:
            return False

        # 寻找Best-fit的空闲块
        best_fit = None
        best_fit_index = -1
        worst_fit = None
        worst_fit_index = -1
        equal = False
        for index, (start, length) in enumerate(self.free_blocks):
            if length == size:  # 找到大小正好匹配的就可以结束遍历
                best_fit = (start, length)
                best_fit_index = index
                self.frag -= 1  # 新分配的空间刚好填满空格，则外部碎片减少一个
                equal = True
                break
            if length > size:
                if best_fit is None or length < best_fit[1]:
                    best_fit = (start, length)
                    best_fit_index = index
                if worst_fit is None or length > worst_fit[1]:
                    worst_fit = (start, length)
                    worst_fit_index = index

        # 如果找不到，说明应该整理内存
        if best_fit is None:
            self.compact()
            self.frag = 1  # 整理过后，外部碎片数应该变为1
            # return False

            # 重新遍历内存
            for index, (start, length) in enumerate(self.free_blocks):
                if length == size:  # 找到大小正好匹配的就可以结束遍历
                    best_fit = (start, length)
                    best_fit_index = index
                    self.frag -= 1  # 新分配的空间刚好填满空格，则外部碎片减少一个
                    equal = True
                    break
                if length > size:
                    if best_fit is None or length < best_fit[1]:
                        best_fit = (start, length)
                        best_fit_index = index
                    if worst_fit is None or length > worst_fit[1]:
                        worst_fit = (start, length)
                        worst_fit_index = index

        if not equal and (best_fit[1] - size) < best_fit[1] * 0.45:
            best_fit = worst_fit
            best_fit_index = worst_fit_index

        # 分配内存
        start, length = best_fit
        # self.allocated_blocks[id] = (start, size, value)
        self.allocated_blocks[id] = {'start':start, 'size':size, 'value':value }
        self.heap[start:start+size] = value[:size]
        self.allocated_size += size

        # 更新空闲块列表
        if length == size:
            self.free_blocks.pop(best_fit_index)
        else:
            self.free_blocks[best_fit_index] = (start + size, length - size)

        return True

    def free(self, id: int) -> bool:
        if id not in self.allocated_blocks:
            return False

        # free操作
        info = self.allocated_blocks.pop(id)
        start = info['start']
        size = info['size']
        self.heap[start:start + size] = ['#'] * size
        self.allocated_size -= size
        self.frag += 1

        # 将此区域添加到空闲块列表
        self.free_blocks.append((start, size))
        self.free_blocks.sort()  # 保持空闲块列表排序，便于合并相邻的空闲块
        self.merge_free_blocks()

        # frags = [i[2] for i in self.free_blocks if i[2] > 0.1 * self.size]

        # 自主整理内存
        # if self.allocated_size > 0.6 * self.size and sum(frags) > 0.25 * (self.size - self.allocated_size):
        #     self.compact()

        if self.frag / self.size > self.ratio:
            self.compact()
            self.frag = 1
            # self.need_compact = True

        return True

    # 合并相邻的空闲块
    def merge_free_blocks(self):
        merged = []
        last = None
        for start, length in self.free_blocks:
            if last and last[0] + last[1] == start:     # 说明前一个空闲块连着下一个空闲块
                last = (last[0], last[1] + length)      # 将两个块合并为一个块last中
                self.frag -= 1
            else:
                if last:
                    merged.append(last)
                last = (start, length)
        if last:
            merged.append(last)
        self.free_blocks = merged

    def data(self) -> dict:
        # return {id: {'start': start, 'size': size , 'value': value} for id, (start, size, value) in self.allocated_blocks.items()}
        return self.allocated_blocks

    def compact(self) -> None:
        offset = 0
        for id in self.allocated_blocks:
            self.allocated_blocks[id]['start'] = offset
            new_offset = offset + self.allocated_blocks[id]['size']
            self.heap[offset:new_offset] = self.allocated_blocks[id]['value']
            offset = new_offset

        self.heap[offset:] = ['#'] * (self.size - offset)
        self.free_blocks = [(offset, self.size - offset)]

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
    else:
        print(dma_test.free(request['id']))

time2 = time.time()
print(f"time is : {time2 - time1}")