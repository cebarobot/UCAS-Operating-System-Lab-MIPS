#include "string.h"

int strlen(char *src)
{
	int i;
	for (i = 0; src[i] != '\0'; i++)
	{
	}
	return i;
}

void memcpy(uint8_t *dest, uint8_t *src, uint32_t len)
{
	for (; len != 0; len--)
	{
		*dest++ = *src++;
	}
}

void memset(void *dest, uint8_t val, uint32_t len)
{
	uint8_t *dst = (uint8_t *)dest;

	for (; len != 0; len--)
	{
		*dst++ = val;
	}
}

void bzero(void *dest, uint32_t len)
{
	memset(dest, 0, len);
}

int strcmp(char *str1, char *str2)
{
	int l1 = strlen(str1);
	int l2 = strlen(str2);
	int i;

	for (i = 0; i < l1 || i < l2; i++)
	{
		if (str1[i] > str2[i])
		{
			return 1;
		}
		else if (str1[i] < str2[i])
		{
			return -1;
		}
	}

	return 0;
}

int memcmp(char *str1, char *str2, uint32_t size)
{
	int i;

	for (i = 0; i < size; i++)
	{
		if (str1[i] > str2[i])
		{
			return 1;
		}
		else if (str1[i] < str2[i])
		{
			return -1;
		}
	}
	return 0;
}

void strcpy(char *dest, char *src)
{
	int l = strlen(src);
	int i;

	for (i = 0; i < l; i++)
	{
		dest[i] = src[i];
	}

	dest[i] = '\0';
}

inline static int atoi_dec(char *str)
{
    int ans = 0;
    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            ans = ans * 10 + *str - '0';
        }
        else
        {
            break;
        }
        str++;
    }
    return ans;
}

inline static int atoi_oct(char *str)
{
    int ans = 0;
    while (*str)
    {
        if (*str >= '0' && *str <= '7')
        {
            ans = ans * 8 + *str - '0';
        }
        else
        {
            break;
        }
        str++;
    }
    return ans;
}

inline static int atoi_hex(char *str)
{
    int ans = 0;
    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            ans = ans * 16 + *str - '0';
        }
        else if (*str >= 'a' && *str <= 'f')
        {
            ans = ans * 16 + *str - 'a' + 10;
        }
        else if (*str >= 'A' && *str <= 'F')
        {
            ans = ans * 16 + *str - 'A' + 10;
        }
        else
        {
            break;
        }
        str++;
    }
    return ans;
}

int atoi(char *str)
{
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        return atoi_hex(str + 2);
    }
    else if (str[0] == '0')
    {
        return atoi_oct(str + 1);
    }
    else
    {
        return atoi_dec(str);
    }
}