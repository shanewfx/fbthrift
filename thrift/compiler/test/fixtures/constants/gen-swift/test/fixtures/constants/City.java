package test.fixtures.constants;

import com.facebook.swift.codec.*;

public enum City
{
    NYC(0), MPK(1), SEA(2), LON(3);

    private final int value;

    City(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }

    public static City fromInteger(int n) {
        switch (n) {
        case 0:
            return NYC;
        case 1:
            return MPK;
        case 2:
            return SEA;
        case 3:
            return LON;
        default:
            return null;
        }
    }
}
