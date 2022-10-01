import random


def generate_new_code(length):
    # generate a numeric code:
    # - with all-different digits,
    # - not starting with '0'
    # - with provided @param length
    res = ""
    availableDigits = ['1', '2', '3', '4', '5', '6', '7', '8', '9']
    x = random.choice(availableDigits)
    res += x
    availableDigits.remove(x)
    availableDigits.append('0')

    for i in range(length - 1):
        if not availableDigits:
            break
        x = random.choice(availableDigits)
        res += x
        availableDigits.remove(x)

    return res


def is_valid_code(code, expected_length):
    return type(code) is str \
           and len(code) == expected_length \
           and code.isdigit() \
           and code[0] != '0'


def create_code_set(size, codeLength):
    s = set()
    collides = 0
    while len(s) < size:
        newCode = generate_new_code(codeLength)
        if newCode in s:
            collides += 1
        else:
            s.add(newCode)
    return (s, collides)
