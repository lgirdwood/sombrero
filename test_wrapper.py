import sys
import os

sys.path.append('python')
import sombrero as smbrr
import ctypes

def main():
    print("Testing libsombrero python wrapper...")
    
    # Check constants
    print(f"SMBRR_MAX_SCALES: {smbrr.SMBRR_MAX_SCALES}")
    
    # Check Structs
    coord = smbrr.SmbrrCoord(x=10, y=20)
    print(f"SmbrrCoord: ({coord.x}, {coord.y})")
    
    # Create simple 1D dummy data to test function calls without needing an image
    width = 100
    stride = width * 4 # float = 4 bytes
    data = (ctypes.c_float * width)()
    
    ctx = smbrr.smbrr.smbrr_new(
        smbrr.SMBRR_DATA_1D_FLOAT, 
        width, 1, stride, 
        smbrr.SMBRR_SOURCE_FLOAT, 
        ctypes.addressof(data)
    )
    
    if ctx:
        print("Successfully created smbrr context via ctypes!")
        smbrr.smbrr.smbrr_free(ctx)
        print("Successfully freed smbrr context.")
    else:
        print("Failed to create smbrr context!")
        sys.exit(1)

if __name__ == "__main__":
    main()
