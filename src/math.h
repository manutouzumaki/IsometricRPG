#include <math.h>

struct Vec2
{
    f32 x;
    f32 y;
};

struct Vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

Vec2 operator+(Vec2 a, Vec2 b)
{
    Vec2 resutl = {};
    resutl.x = a.x + b.x;
    resutl.y = a.y + b.y;
    return resutl;
}

Vec2 operator-(Vec2 a, Vec2 b)
{
    Vec2 resutl = {};
    resutl.x = a.x - b.x;
    resutl.y = a.y - b.y;
    return resutl;
}

Vec2 operator*(Vec2 v, f32 a)
{
    Vec2 resutl = {};
    resutl.x = v.x * a;
    resutl.y = v.y * a;
    return resutl;
}

Vec2 operator/(Vec2 v, f32 a)
{
    Vec2 resutl = {};
    resutl.x = v.x / a;
    resutl.y = v.y / a;
    return resutl;
}

Vec2 Vec2ElementMul(Vec2 a, Vec2 b)
{
    Vec2 result = {};
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

f32 Vec2Dot(Vec2 a, Vec2 b)
{
    f32 result = (a.x*b.x) + (a.y*b.y);
    return result;
}

f32 Vec2Length(Vec2 v)
{
    f32 result = sqrtf(Vec2Dot(v, v));
    return result;
}

f32 Vec2LengthSq(Vec2 v)
{
    f32 result = Vec2Dot(v, v);
    return result;
}

void Vec2Normalize(Vec2 *v)
{
    f32 length = Vec2Length(*v);
    v->x /= length;
    v->y /= length;
}

Vec2 Vec2Norm(Vec2 v)
{
    f32 length = Vec2Length(v);
    Vec2 result = {};
    result.x = v.x / length;
    result.y = v.y / length;   
    return result;
}

Vec2 Vec2Rotate(Vec2 v, f32 angle)
{
    Vec2 result = {};
    result.x = v.x*cosf(angle) - v.y*sinf(angle);
    result.y = v.x*sinf(angle) + v.y*cosf(angle);
    return result;
}

Vec2 Vec2Perp(Vec2 v)
{
    Vec2 result = {};
    result.x = -v.y;
    result.y = v.x;
    return result;
}

Vec2 Vec2Floor(Vec2 v)
{
    Vec2 result = {};
    result.x = floorf(v.x);
    result.y = floorf(v.y);
    return result;
}

f32 DegToRad(f32 degree)
{
    f32 rad = degree * (PI/180.0f);
    return rad;
}

f32 RadToDeg(f32 rad)
{
    f32 degree = rad * (180.0f/PI);
    return degree;
}

f32 Square(f32 a)
{
    f32 result = a*a;
    return result;
}

Vec4 Vec4Lerp(Vec4 a, Vec4 b, f32 t)
{
    Vec4 result = 
    {
        (1.0f - t)*b.x + t*a.x,
        (1.0f - t)*b.y + t*a.y,
        (1.0f - t)*b.z + t*a.z,
        (1.0f - t)*b.w + t*a.w
    };
    return result;
}


