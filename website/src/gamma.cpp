unsigned char
gamma (float x)
{
    x = pow (5.5555f * max (0.f, x), 0.4545f) * 84.66f;
    return (unsigned char) clamp (x, 0.f, 255.f);
}

