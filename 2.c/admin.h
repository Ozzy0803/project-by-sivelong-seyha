#ifndef ADMIN_H
#define ADMIN_H

#include "struct.h"


void seedAdminIfNeeded(void) {
    for(int i = 0; i < userCount; i++) {
        if(users[i].role == ADMIN) return;
    }

    /* create default admin */
    User *a = &users[userCount++];
    a->id = 1000;
    strcpy(a->name,     "Admin");
    strcpy(a->phone,    "000");
    strcpy(a->password, "admin123");
    a->role    = ADMIN;
    a->balance = 0;
    a->rating  = 5.0f;
    saveData();
}

void adminViewAllUsers(void) {
    printf("\n%-5s %-20s %-15s %-10s %-8s %-6s\n","ID","Name","Phone","Role","Balance","Rides");
    printf("----------------------------------------------------------------------\n");
    for(int i = 0; i < userCount; i++){
        User *u = &users[i];
        const char *role = u->role == ADMIN ? "Admin" : u->role == DRIVER ? "Driver" : "Passenger";
        printf("%-5d %-20s %-15s %-10s $%-7.2f %-6d\n",
               u->id, u->name, u->phone, role, u->balance, u->totalRides);
    }
}

void adminViewAllDrivers(void) {
    printf("\n%-5s %-20s %-12s %-20s %-8s %-7s %-6s\n","ID","Name","Phone","Vehicle","Status","Earn","Trips");
    printf("------------------------------------------------------------------------------\n");
    for(int i = 0; i < driverCount; i++){
        Driver *d = &drivers[i];
        printf("%-5d %-20s %-12s %-20s %-8s $%-6.2f %-6d\n",
               d->id, d->name, d->phone, d->vehicle,
               driverStatusName(d->status), d->earnings, d->totalTrips);
    }
}

void adminViewAllBookings(void) {
    printf("\n%-6s %-6s %-6s %-10s %-12s %-6s %-10s\n","BID","UID","DID","Status","Type","km","Fare");
    printf("----------------------------------------------------------------\n");
    for(int i = 0; i < bookingCount; i++){
        Booking *b = &bookings[i];
        printf("%-6d %-6d %-6d %-10s %-12s %-6.1f $%.2f\n",
               b->id, b->userId, b->driverId,
               bookingStatusName(b->status), rideTypeName(b->rideType),
               b->distance, b->fare);
    }
}

void adminStats(void) {
    float totalRevenue = 0.0f;
    float totalDriverPay = 0.0f;
    int completed = 0, cancelled = 0, pending = 0;

    for(int i = 0; i < bookingCount; i++){
        switch(bookings[i].status){
            case COMPLETED: totalRevenue += bookings[i].fare; completed++; break;
            case CANCELLED: cancelled++; break;
            case PENDING:   pending++;   break;
            default: break;
        }
    }
    for(int i = 0; i < driverCount; i++) {
        totalDriverPay += drivers[i].earnings;
    }

    printf("\n========== PLATFORM STATISTICS ==========\n");
    printf("Total Users      : %d\n", userCount);
    printf("Total Drivers    : %d\n", driverCount);
    printf("Total Bookings   : %d\n", bookingCount);
    printf("  Completed      : %d\n", completed);
    printf("  Cancelled      : %d\n", cancelled);
    printf("  Pending        : %d\n", pending);
    printf("Total Revenue    : $%.2f\n", totalRevenue);
    printf("Driver Payouts   : $%.2f\n", totalDriverPay);
    printf("Platform Profit  : $%.2f\n", totalRevenue - totalDriverPay);
    printf("==========================================\n");
}

void adminDeleteUser(void) {
    printf("Enter User ID to delete: ");
    int uid; scanf("%d", &uid);
    for(int i = 0; i < userCount; i++){
        if(users[i].id == uid){
            if(users[i].role == ADMIN){
                printf("Cannot delete admin.\n");
                return;
            }
            for(int j = i; j < userCount - 1; j++)
                users[j] = users[j + 1];
            userCount--;
            saveData();
            printf("User deleted.\n");
            return;
        }
    }
    printf("User not found.\n");
}

void adminMenu(void) {
    int choice;
    do {
        clearScreen();
        printf("╔══════════════════════════════╗\n");
        printf("║   RIDE APP — ADMIN PANEL     ║\n");
        printf("╚══════════════════════════════╝\n\n");
        printf("  1. View All Users\n");
        printf("  2. View All Drivers\n");
        printf("  3. View All Bookings\n");
        printf("  4. Platform Statistics\n");
        printf("  5. Delete User\n");
        printf("  0. Logout\n");
        printf("\nChoice: ");
        scanf("%d", &choice);
        switch(choice){
            case 1: adminViewAllUsers();    pauseOnly(); break;
            case 2: adminViewAllDrivers();  pauseOnly(); break;
            case 3: adminViewAllBookings(); pauseOnly(); break;
            case 4: adminStats();           pauseOnly(); break;
            case 5: adminDeleteUser();      pauseOnly(); break;
            case 0: isAdmin = 0; printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while(choice != 0);
}

#endif // ADMIN_H


