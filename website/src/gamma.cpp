unsigned char
gamma (float x)
{
    x = std::pow (5.5555f * std::max (0.f, x), 0.4545f) * 84.66f;
    return (unsigned char) std::clamp (x, 0.f, 255.f);
}

