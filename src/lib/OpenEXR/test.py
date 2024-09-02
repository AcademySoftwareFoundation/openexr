import sys
import random
import math


HALF_SIZE = 2
SINGLE_SIZE = 4


class buf_t:
    def __init__(self, buf=[], size=0, pos=0) -> None:
        self.buf: list = buf
        self.size: int = size
        self.pos: int = pos


def bufcpy(dst: buf_t, dsti: int, src: buf_t, srci: int, copy_size: int) -> None:
    memcpy(dst.buf, dsti, src.buf, srci, copy_size)
    dst.size += copy_size
    src.pos += copy_size
    dst.pos += copy_size


class Channel:
    """Channel declaration including name, sample byte size and sub-sampling.
    """
    def __init__(self, name, byte_size, x_sampling=1, y_sampling=1) -> None:
        self.name = name
        self.bytes = byte_size
        self.xs = x_sampling
        self.ys = y_sampling


class BufParams:
    def __init__(self) -> None:
        self.channel_count = 0
        self.channels_size = []
        self.line_count = 0
        self.sample_count_per_line = []
        self.total_sample_count = 0

    def add_channel_count(self, count):
        if not self.channel_count:
            self.channel_count = count

    def add_channel_size(self, size):
        if len(self.channels_size) < self.channel_count:
            self.channels_size.append(size)

    def add_line_count(self, count):
        if not self.line_count:
            self.line_count = count

    def add_line_sample_count(self, count):
        if len(self.sample_count_per_line) < self.line_count:
            self.sample_count_per_line.append(count)
            self.total_sample_count += count

    def __repr__(self) -> str:
        r = [
            f"{self.__class__}:",
            f" |         channel_count = {self.channel_count}",
            f" |         channels_size = {self.channels_size}",
            f" |            line_count = {self.line_count}",
            f" | sample_count_per_line = {self.sample_count_per_line}",
            f" |    total_sample_count = {self.total_sample_count}",
        ]
        return "\n".join(r)


def mk_buf(*channels, num_lines=5, num_pixels=5, deep=False, seed=31):
    buf = ""
    buf_params = BufParams()
    buf_params.add_line_count(num_lines)
    ch_size_list = list(channels)
    buf_params.add_channel_count(len(ch_size_list))
    random.seed(seed)
    for y in range(num_lines):
        # print(f"y = {y}")
        pixel_samples = [random.randint(1, 3) if deep else 1 for x in range(num_pixels)]
        n_samples = sum(pixel_samples)
        buf_params.add_line_sample_count(n_samples)
        for chan in ch_size_list:
            if y % chan.ys != 0:
                continue
            # sys.stdout.write(f"  {chan.name}: ")
            buf_params.add_channel_size(chan.bytes)
            fmt = "%s%d" if chan.bytes == 2 else "%s%03d"
            for x in range(num_pixels):
                if x % chan.xs != 0:
                    continue
                # sys.stdout.write(f"{x} ")
                tok = fmt % (chan.name, x)
                buf += tok * pixel_samples[x]
            # sys.stdout.write("\n")
    return buf, buf_params


def print_buf(buf, name):
    print(f"{name}:")
    p = 0
    last_ch = buf[0]
    last_run = ""
    sys.stdout.write(f"{0 : >4}: ")
    while p < len(buf):
        if buf[p] not in "0123456789_" and buf[p] != last_ch:
            sys.stdout.write(f"  ({len(last_run)})\n")
            sys.stdout.write(f"{p : >4}: ")
            last_ch = buf[p]
            last_run = ""
        sys.stdout.write(buf[p])
        last_run += buf[p]
        p += 1
    sys.stdout.write(f"  ({len(last_run)})\n\n")


def memcpy(dst, dsti, src, srci, size, debug=False):
    if debug:
        print(
            "DEBUG:  memcpy(dst, %d, src, %d, %d)  src = %r . dst = %r"
            % (
                dsti,
                srci,
                size,
                src[srci : srci + size],
                to_str(dst[dsti : dsti + size]),
            )
        )
    for i in range(size):
        try:
            dst[dsti + i] = src[srci + i]
        except IndexError:
            print("dst[%d] idx = %d" % (len(dst), dsti + i))
            print("src[%d] idx = %d" % (len(src), srci + i))
            raise


def malloc(size, zero=False):
    if zero:
        return list([0] * size)
    return list(["_"] * size)


def to_str(buf):
    return "".join([str(x) for x in buf])


def cumulative_samples_per_line(sample_count_per_line):
    scpl_cum = malloc(len(sample_count_per_line) + 1, zero=True)
    for i in range(len(sample_count_per_line)):
        scpl_cum[i + 1] = scpl_cum[i] + sample_count_per_line[i]
    print(f"scpl_cum = {scpl_cum}")
    return scpl_cum


def channel_offsets(channels_size, buf_sample_count):
    # count the number of half and single channels
    n_half = 0
    n_single = 0
    for ch in channels_size:
        if ch == HALF_SIZE:
            n_half += 1
        else:
            n_single += 1
    print(f"n_half = {n_half}   n_single = {n_single}")

    # map offsets to channel numbers
    nh = 0
    ns = 0
    half_ch_size = buf_sample_count * HALF_SIZE
    out_split = n_half * half_ch_size
    ch_offsets = malloc(len(channels_size), zero=True)  # malloc
    for i, size in enumerate(channels_size):
        if size == HALF_SIZE:
            ch_offsets[i] = half_ch_size * nh
            nh += 1
        elif size == SINGLE_SIZE:
            ch_offsets[i] = out_split + half_ch_size * 2 * ns
            ns += 1

    print(f"ch_offsets = {ch_offsets}")
    print(f"out_split = {out_split}")

    return ch_offsets, out_split


# return a single buffer and the split position
def to_planar_3(
    in_buf,
    channel_count,
    channels_size,
    line_count,
    sample_count_per_line,
):
    print("=" * 180)

    out_buf = malloc(len(in_buf))
    out_split = 0
    print_buf(in_buf, "to_planar_3: in_buf")

    # cumulative sample/line table
    scpl_cum = cumulative_samples_per_line(sample_count_per_line)
    buf_sample_count = scpl_cum[line_count]
    print(f"buf_sample_count = {buf_sample_count}")

    print(f"channels_size = {channels_size}")
    ch_offsets, out_split = channel_offsets(channels_size, buf_sample_count)
    print()

    # copy data to output buffer
    in_pos = 0
    for ln in range(line_count):
        for ch in range(channel_count):
            copy_size = channels_size[ch] * sample_count_per_line[ln]
            out_pos = ch_offsets[ch] + scpl_cum[ln] * channels_size[ch]
            memcpy(out_buf, out_pos, in_buf, in_pos, copy_size, debug=False)
            in_pos += copy_size
            # buf = to_str(out_buf)
            # print_buf(buf, f"line {ln} ch {ch}")

    out_buf = to_str(out_buf)
    assert len(in_buf) == len(out_buf)

    print_buf(out_buf, "to_planar_3: out_buf")
    return out_buf, out_split


def from_planar_3(
    in_buf,
    channel_count,
    channels_size,
    line_count,
    sample_count_per_line,
):
    print("=" * 180)

    print_buf(in_buf, "from_planar_3: in_buf")
    out_buf = malloc(len(in_buf))

    print(f"sample_count_per_line = {sample_count_per_line}")
    scpl_cum = cumulative_samples_per_line(sample_count_per_line)
    buf_sample_count = scpl_cum[line_count]
    print(f"buf_sample_count = {buf_sample_count}")

    print(f"channels_size = {channels_size}")
    ch_offsets, out_split = channel_offsets(channels_size, buf_sample_count)
    print()

    out_pos = 0
    for ln in range(line_count):
        for ch in range(channel_count):
            copy_size = channels_size[ch] * sample_count_per_line[ln]
            in_pos = ch_offsets[ch] + scpl_cum[ln] * channels_size[ch]
            memcpy(out_buf, out_pos, in_buf, in_pos, copy_size, debug=False)
            out_pos += copy_size

    out_buf = to_str(out_buf)
    print_buf(out_buf, "from_planar_3: out_buf")

    return out_buf


buf, params = mk_buf(
    Channel("r", HALF_SIZE),
    Channel("g", HALF_SIZE),
    Channel("b", SINGLE_SIZE),
    Channel("a", HALF_SIZE),
    Channel("i", SINGLE_SIZE),
    num_lines=5,
    num_pixels=5,
    deep=True,
)
print(params)
p_buf, split_pos = to_planar_3(
    buf,
    params.channel_count,  # channel count
    params.channels_size,  # channels size
    params.line_count,  # line count
    params.sample_count_per_line,  # sample count per line
)
i_buf = from_planar_3(
    p_buf,
    params.channel_count,  # channel count
    params.channels_size,  # channels size
    params.line_count,  # line count
    params.sample_count_per_line,  # sample count per line
)
assert buf == i_buf, "buf and i_buf must be the same !"

buf, params = mk_buf(
    Channel("Y", HALF_SIZE),
    Channel("R", HALF_SIZE, 2, 2),
    Channel("B", HALF_SIZE, 2, 2),
    Channel("a", HALF_SIZE),
    Channel("i", SINGLE_SIZE),
    num_lines=5,
    num_pixels=5,
    deep=False,
)
print_buf(buf, "sub_buf")
