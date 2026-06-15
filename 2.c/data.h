#ifndef DATA_H
#define DATA_H

#include "struct.h"

#define DATA_FILE "ride_data.dat"

void saveData(void) {
    FILE *f = fopen(DATA_FILE, "wb");
    if(!f){ printf("Warning: could not save data.\n"); return; }
    fwrite(&userCount,    sizeof(int), 1, f);
    fwrite(&driverCount,  sizeof(int), 1, f);
    fwrite(&bookingCount, sizeof(int), 1, f);
    fwrite(users,    sizeof(User),    userCount,    f);
    fwrite(drivers,  sizeof(Driver),  driverCount,  f);
    fwrite(bookings, sizeof(Booking), bookingCount, f);
    fclose(f);
}

void loadData(void) {
    FILE *f = fopen(DATA_FILE, "rb");
    if(!f) return;
    fread(&userCount,    sizeof(int), 1, f);
    fread(&driverCount,  sizeof(int), 1, f);
    fread(&bookingCount, sizeof(int), 1, f);
    fread(users,    sizeof(User),    userCount,    f);
    fread(drivers,  sizeof(Driver),  driverCount,  f);
    fread(bookings, sizeof(Booking), bookingCount, f);
    fclose(f);
}

#endif // DATA_H
 