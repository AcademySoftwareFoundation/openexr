import sys
import random
from typing import Union

# NOTE: coding style deliberately C-like and un-pythonic

HALF_SIZE = 2
SINGLE_SIZE = 4


class Channel:
    """Channel declaration including name, sample byte size and sub-sampling."""

    def __init__(
        self, name: str, byte_size: int, x_sampling: int = 1, y_sampling: int = 1
    ) -> None:
        self.name: str = name
        self.bytes: int = byte_size
        self.xs: int = x_sampling
        self.ys: int = y_sampling

    def __repr__(self) -> str:
        r = [
            f"{self.__class__}:",
            f" name = {self.name}",
            f" bytes = {self.bytes}",
            f" xs = {self.xs}",
            f" ys = {self.ys}",
        ]
        return ",".join(r)


class BufParams:
    def __init__(self, channels: list, line_count: int, pixel_count: int) -> None:
        self.channels: list = channels
        self.channel_count: int = len(self.channels)
        self.line_count: int = line_count
        self.pixel_count: int = pixel_count
        self.sample_count_per_line: list[int] = []
        self.line_samples: list[list[int]] = []
        self.total_sample_count: int = 0

    def add_line_sample_count(self, count: int) -> None:
        if len(self.sample_count_per_line) < self.line_count:
            self.sample_count_per_line.append(count)
            self.total_sample_count += count

    def __repr__(self) -> str:
        nl = "\n |"
        spc = " "
        r = [
            f"{self.__class__}:",
            f" |         channel_count = {self.channel_count}",
            f" |             channels = {(nl+spc*24).join([str(ch) for ch in self.channels])}",
            f" |            line_count = {self.line_count}",
            f" |           pixel_count = {self.pixel_count}",
            f" | sample_count_per_line = {self.sample_count_per_line}",
            f" |          line_samples = {self.line_samples}",
            f" |    total_sample_count = {self.total_sample_count}",
        ]
        return "\n".join(r)


def mk_buf(
    *channels, num_lines=5, num_pixels=5, deep=False, seed=31
) -> tuple[str, BufParams]:
    buf = ""
    buf_params = BufParams(list(channels), num_lines, num_pixels)
    random.seed(seed)

    for y in range(num_lines):
        # generate samples for each pixel of this line
        pixel_samples = [random.randint(1, 3) if deep else 1 for x in range(num_pixels)]
        n_samples = sum(pixel_samples)
        buf_params.add_line_sample_count(n_samples)
        buf_params.line_samples.append(pixel_samples)

        for chan in buf_params.channels:
            if y % chan.ys != 0:
                continue
            # sys.stdout.write(f"  {chan.name}: ")
            fmt = "%s%d" if chan.bytes == 2 else "%s%03d"
            for x in range(num_pixels):
                if x % chan.xs != 0:
                    continue
                # sys.stdout.write(f"{x} ")
                tok = fmt % (chan.name, x)
                buf += tok * pixel_samples[x]
            # sys.stdout.write("\n")
    return buf, buf_params


def print_buf(buf, name) -> None:
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
    sys.stdout.write(f"  ({len(last_run)})\n")
    sys.stdout.write(f"buffer size:  {len(buf)}\n\n")


def memcpy(
    dst: list,
    dsti: int,
    src: Union[list, str],
    srci: int,
    size: int,
    debug: bool = False,
) -> None:
    """Simulate a c-style memcpy, using lists.

    Args:
        dst (list): destination list
        dsti (int): write offset in destination list
        src (list): source list
        srci (int): read offset in source list
        size (int): the amount of data to copy
        debug (bool, optional): verbosity control. Defaults to False.
    """
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
            print(
                "memcpy IndexError: dst[%d] requested idx = %d" % (len(dst), dsti + i)
            )
            print(
                "memcpy IndexError: src[%d] requested idx = %d" % (len(src), srci + i)
            )
            raise


def malloc(size: int, zero: bool = False) -> list:
    """simulate a c-style malloc, with optional zeroing.

    Args:
        size (int): requested list size.
        zero (bool, optional): fill with 0 instead of "_". Defaults to False.

    Returns:
        list: a list that can by used by memcpy and friends.
    """
    if zero:
        return list([0] * size)
    return list(["_"] * size)


def to_str(buf: list) -> str:
    return "".join([str(x) for x in buf])


def cumulative_samples_per_line(sample_count_per_line: list[int]) -> list[int]:
    """Return a monotonically increasing list of samples per line, irrespective
    of channel sub-sampling params.

    Args:
        sample_count_per_line (list): number of samples for each line.

    Returns:
        list: cumulative number of samples
    """
    scpl_cum: list = malloc(len(sample_count_per_line) + 1, zero=True)
    for i in range(len(sample_count_per_line)):
        scpl_cum[i + 1] = scpl_cum[i] + sample_count_per_line[i]
    print(f"scpl_cum = {scpl_cum}")
    return scpl_cum


def channel_offsets(
    channels_size: list[int], buf_sample_count: int
) -> tuple[list[int], int]:
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
    in_buf: str,  # list
    channel_count: int,  # number of channels
    channels_size: list[int],  # list of channels
    line_count: int,  # number of lines
    sample_count_per_line: list[int],  # num of samples per line when xsamp=1
) -> tuple[str, int]:
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
    in_buf: str,
    channel_count: int,
    channels_size: list[int],
    line_count: int,
    sample_count_per_line: list[int],
) -> str:
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


# -----------------------------------------------------------------------------


class Mem:
    """Simulate a chunk of allocated memory in C, mostly for bound-checking."""

    def __init__(self, data=None) -> None:
        self.data = []
        if data:
            if isinstance(data, list):
                self.data = data
            elif isinstance(data, str):
                self.data = list(data)
            else:
                raise TypeError("Un-supported data type !")

    def allocate(self, size) -> None:
        self.data = [0] * size

    def __setitem__(self, idx, value) -> None:
        if idx >= len(self.data):
            raise IndexError(
                f"Out of bounds write: index = {idx} . alloc = {len(self.data)}"
            )
        self.data[idx] = value

    def __getitem__(self, idx):
        if idx >= len(self.data):
            raise IndexError(
                f"Out of bounds read: index = {idx} . alloc = {len(self.data)}"
            )
        return self.data[idx]

    def __len__(self) -> int:
        return len(self.data)

    def __repr__(self) -> str:
        return str(self.data)

    def __str__(self) -> str:
        return "".join([str(x) for x in self.data])


class Pointer:
    """Simulate a C pointer to a chunk of allocated memory."""

    def __init__(self, mem: Mem, pos: int = 0) -> None:
        self.mem: Mem = mem
        self.pos: int = pos

    def __iadd__(self, x: int) -> "Pointer":
        self.pos += x
        return self


def memcpy2(dst: Pointer, src: Pointer, size: int):
    """Simulate a C memcopy

    Args:
        dst (Pointer): The pointer to write to
        src (Pointer): The pointer to read from
        size (int): Amount of data to copy
    """
    for i in range(size):
        dst.mem[dst.pos + i] = src.mem[src.pos + i]


def channel_samples(ch: Channel, line_samples: list[list[int]]) -> int:
    """Compute the number of sample for this channel, accounting for
    subsampling and deep.

    Args:
        ch (Channel): the channel obj
        line_samples (list[list[int]]): the sample table

    Returns:
        int: the actual number of samples
    """
    num_samples: int = 0
    for ln, ls in enumerate(line_samples):
        if ln % ch.ys != 0:
            continue
        for p, ns in enumerate(ls):
            if p % ch.xs == 0:
                num_samples += ns
    return num_samples


def compute_channel_offsets(
    num_channels: int,
    channels: list[Channel],
    x_res: int,
    y_res: int,
    line_samples: list[list[int]],
) -> tuple[list[int], int]:
    """Compute the offset of each channel in a planar layout.

    Args:
        num_channels (int): number of channels
        channels (list[Channel]): all available channels
        x_res (int): number of lines in buffer
        y_res (int): number of pixels per line
        line_samples (list[list[int]]): sample table.

    Returns:
        tuple(list, int): the channel offsets and the position of single
                          precision data.
    """

    # map offsets to channel numbers
    out_split = 0
    ofst = 0
    ch_offsets: list = malloc(num_channels, zero=True)  # malloc

    for byte_size in (HALF_SIZE, SINGLE_SIZE):
        out_split = ofst
        for i, ch in enumerate(channels):
            if ch.bytes != byte_size:
                continue
            num_samples = channel_samples(ch, line_samples)
            ch_offsets[i] = ofst
            ofst += ch.bytes * num_samples

    print(f"ch_offsets = {ch_offsets}")
    print(f"out_split = {out_split}")

    return ch_offsets, out_split


def unpack(
    in_buf: str,
    in_size: int,
    num_channels: int,
    channels: list[Channel],
    x_res: int,
    y_res: int,
    line_samples: list[list[int]],
) -> tuple[str, int]:
    """Unpack per-line interleaved and sub-sampled pixel data, into a half and
    single precision section.

    Example with 4 pixels x 2 lines of rGb (half, single, half)
    packed per line:              unpacked per channel:
    [rrrrGGGGbbbbrrrrGGGGbbbb] -> [rrrrrrrrbbbbbbbbGGGGGGGG]

    Args:
        in_buf (list): packed buffer
        in_size (int): size of in_buf
        num_channels (int): the number of channels
        channels (list[Channel]): channel list
        x_res (int): number of pixels per line
        y_res (int): number of lines
        line_samples (list[list[int]]): list of samples per line

    Returns:
        tuple(list, int): unpacked buffer and position of single precision data.
    """
    print_buf(in_buf, "unpack: in_buf")

    out_buf = Mem()
    out_buf.allocate(in_size)
    split_pos = 0

    # compute channel offsets in unpacked buffer, considering per-channel subsampling
    ch_offsets, split_pos = compute_channel_offsets(
        num_channels, channels, x_res, y_res, line_samples
    )

    src_ptr = Pointer(Mem(in_buf), 0)
    for line in range(y_res):
        for ch in range(num_channels):
            chan: Channel = channels[ch]
            if line % chan.ys != 0:
                continue
            for pixel in range(x_res):
                if pixel % chan.xs != 0:
                    continue
                copy_size = line_samples[line][pixel] * chan.bytes
                dst_ptr = Pointer(out_buf, ch_offsets[ch])
                memcpy2(dst_ptr, src_ptr, copy_size)
                ch_offsets[ch] += copy_size
                src_ptr += copy_size
            # print_buf(str(out_buf), f"l={line} . ch={ch}")

    out_buf = str(out_buf)
    assert len(in_buf) == len(out_buf)

    print_buf(out_buf, "unpack: out_buf")
    return out_buf, split_pos


def pack(
    in_buf: str,
    in_size: int,
    num_channels: int,
    channels: list[Channel],
    x_res: int,
    y_res: int,
    line_samples: list[list[int]],
) -> str:
    """re-pack per-channel buffer.

    Example with 4 pixels x 2 lines of rGb (half, single, half)
    unpacked per channel:         packed per line:
    [rrrrrrrrbbbbbbbbGGGGGGGG] -> [rrrrGGGGbbbbrrrrGGGGbbbb]

    Args:
        in_buf (str): _description_
        in_size (int): _description_
        num_channels (int): _description_
        channels (list[Channel]): _description_
        x_res (int): _description_
        y_res (int): _description_
        line_samples (list[list[int]]): _description_

    Returns:
        str: _description_
    """
    out_buf = Mem()
    out_buf.allocate(in_size)

    ch_offsets, split_pos = compute_channel_offsets(
        num_channels, channels, x_res, y_res, line_samples
    )

    ibuf = Mem(in_buf)
    dst_pointer = Pointer(out_buf)
    for line in range(y_res):
        for ch in range(num_channels):
            chan = channels[ch]
            if line % chan.ys != 0:
                continue
            for pixel in range(x_res):
                if pixel % chan.xs == 0:
                    copy_size = line_samples[line][pixel] * chan.bytes
                    src_ptr = Pointer(ibuf, ch_offsets[ch])
                    memcpy2(dst_pointer, src_ptr, copy_size)
                    dst_pointer += copy_size
                    ch_offsets[ch] += copy_size

    out_buf = str(out_buf)
    print_buf(out_buf, "pack: out_buf")
    return out_buf


# print(
#     "NON-SUBSAMPLING DEEP BUFFER ----------------------------------------------------------"
# )
# buf, params = mk_buf(
#     Channel("r", HALF_SIZE),
#     Channel("g", HALF_SIZE),
#     Channel("b", SINGLE_SIZE),
#     Channel("a", HALF_SIZE),
#     Channel("i", SINGLE_SIZE),
#     num_lines=5,
#     num_pixels=5,
#     deep=True,
# )
# print(params)
# p_buf, split_pos = to_planar_3(
#     buf,
#     params.channel_count,  # channel count
#     [ch.bytes for ch in params.channels],  # channels size
#     params.line_count,  # line count
#     params.sample_count_per_line,  # sample count per line
# )
# i_buf = from_planar_3(
#     p_buf,
#     params.channel_count,  # channel count
#     [ch.bytes for ch in params.channels],  # channels size
#     params.line_count,  # line count
#     params.sample_count_per_line,  # sample count per line
# )
# assert buf == i_buf, "buf and i_buf must be the same !"

tests = {
    "rGbaI": {
        "chans": (
            Channel("r", HALF_SIZE),
            Channel("G", SINGLE_SIZE),
            Channel("b", HALF_SIZE),
            Channel("a", HALF_SIZE),
            Channel("I", SINGLE_SIZE),
        ),
        "deep": False,
    },
    "rGbaI deep": {
        "chans": (
            Channel("r", HALF_SIZE),
            Channel("G", SINGLE_SIZE),
            Channel("b", HALF_SIZE),
            Channel("a", HALF_SIZE),
            Channel("I", SINGLE_SIZE),
        ),
        "deep": True,
    },
    "yrbaI": {
        "chans": (
            Channel("y", HALF_SIZE),
            Channel("r", HALF_SIZE, 2, 2),
            Channel("b", HALF_SIZE, 2, 2),
            Channel("a", HALF_SIZE),
            Channel("I", SINGLE_SIZE),
        ),
        "deep": False,
    },
    "yrbaI deep": {
        "chans": (
            Channel("y", HALF_SIZE),
            Channel("r", HALF_SIZE, 2, 2),
            Channel("b", HALF_SIZE, 2, 2),
            Channel("a", HALF_SIZE),
            Channel("I", SINGLE_SIZE),
        ),
        "deep": True,
    },
    "AbCdeF deep": {
        "chans": (
            Channel("A", SINGLE_SIZE),
            Channel("b", HALF_SIZE, 2, 2),
            Channel("C", SINGLE_SIZE, 2, 1),
            Channel("d", HALF_SIZE, 1, 2),
            Channel("e", HALF_SIZE),
            Channel("F", SINGLE_SIZE),
        ),
        "deep": True,
    },
}
for name, tparams in tests.items():
    tname = f"TEST: {name} "
    tname += "-" * (100 - len(tname))
    print(f"\n{tname}")
    buf, params = mk_buf(
        *tparams["chans"],
        num_lines=5,
        num_pixels=5,
        deep=tparams["deep"],
    )
    print(params)
    p_buf, split_pos = unpack(
        buf,
        len(buf),
        params.channel_count,
        params.channels,
        params.pixel_count,
        params.line_count,
        params.line_samples,
    )
    i_buf = pack(
        p_buf,
        len(buf),
        params.channel_count,
        params.channels,
        params.pixel_count,
        params.line_count,
        params.line_samples,
    )
    assert buf == i_buf, "buf and i_buf must be the same !"
