#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ============================================================
//  CONSTANTS & LIMITS
// ============================================================
#define MAX_USERS       100
#define MAX_DRIVERS     100
#define MAX_BOOKINGS    500
#define MAX_NAME        50
#define MAX_PHONE       15
#define MAX_PASS        30
#define MAX_PLATE       10
#define MAX_VEHICLE     30
#define DATA_FILE       "ride_data.dat"

// ============================================================
//  ENUMS
// ============================================================
typedef enum { PASSENGER = 1, DRIVER, ADMIN } UserRole;
typedef enum { AVAILABLE = 1, ON_TRIP, OFFLINE } DriverStatus;
typedef enum { PENDING = 1, ACCEPTED, IN_PROGRESS, COMPLETED, CANCELLED } BookingStatus;
typedef enum { ECONOMY = 1, COMFORT, XL } RideType;

// ============================================================
//  STRUCTS
// ============================================================
typedef struct {
    int    id;
    char   name[MAX_NAME];
    char   phone[MAX_PHONE];
    char   password[MAX_PASS];
    UserRole role;
    float  balance;
    float  rating;
    int    totalRides;
} User;

typedef struct {
    int    id;
    char   name[MAX_NAME];
    char   phone[MAX_PHONE];
    char   password[MAX_PASS];
    char   vehicle[MAX_VEHICLE];
    char   plate[MAX_PLATE];
    DriverStatus status;
    float  rating;
    int    totalTrips;
    float  earnings;
    float  lat, lon;   /* simulated location */
} Driver;

typedef struct {
    int    id;
    int    userId;
    int    driverId;    /* -1 if not yet assigned */
    char   pickupLoc[60];
    char   dropLoc[60];
    float  pickupLat, pickupLon;
    float  dropLat,   dropLon;
    float  distance;   /* km */
    float  fare;
    RideType  rideType;
    BookingStatus status;
    char   createdAt[25];
    int    userRating;   /* 1-5, 0 = not rated */
    int    driverRating;
} Booking;

// ============================================================
//  GLOBAL DATA
// ============================================================
User    users[MAX_USERS];
Driver  drivers[MAX_DRIVERS];
Booking bookings[MAX_BOOKINGS];
int     userCount    = 0;
int     driverCount  = 0;
int     bookingCount = 0;
int     loggedInUser = -1;   /* index into users[]  */
int     loggedInDriver = -1; /* index into drivers[] */
int     isAdmin      = 0;

// ============================================================
//  UTILITY
// ============================================================
void clearScreen() { printf("\n\n"); }
void pause()       { printf("\nPress Enter to continue..."); getchar(); getchar(); }
void pauseOnly()   { printf("\nPress Enter to continue..."); getchar(); }

void getTimestamp(char *buf) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, 25, "%Y-%m-%d %H:%M:%S", tm);
}

/* Euclidean distance turned into ~km */
float calcDistance(float lat1, float lon1, float lat2, float lon2) {
    float dlat = lat2 - lat1, dlon = lon2 - lon1;
    return (float)sqrt(dlat*dlat + dlon*dlon) * 111.0f;
}

float calcFare(float dist, RideType rt) {
    float base, perKm;
    switch(rt) {
        case COMFORT: base=2.5f; perKm=1.2f; break;
        case XL:      base=3.0f; perKm=1.5f; break;
        default:      base=1.5f; perKm=0.8f; break; /* ECONOMY */
    }
    float fare = base + dist * perKm;
    return fare < base ? base : fare;
}

const char* rideTypeName(RideType rt) {
    switch(rt){ case ECONOMY: return "Economy"; case COMFORT: return "Comfort"; case XL: return "XL"; }
    return "Unknown";
}
const char* bookingStatusName(BookingStatus s) {
    switch(s){ case PENDING: return "Pending"; case ACCEPTED: return "Accepted";
               case IN_PROGRESS: return "In Progress"; case COMPLETED: return "Completed";
               case CANCELLED: return "Cancelled"; }
    return "Unknown";
}
const char* driverStatusName(DriverStatus s) {
    switch(s){ case AVAILABLE: return "Available"; case ON_TRIP: return "On Trip"; case OFFLINE: return "Offline"; }
    return "Unknown";
}

// ============================================================
//  FILE PERSISTENCE
// ============================================================
void saveData() {
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

void loadData() {
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

void seedAdminIfNeeded() {
    for(int i=0;i<userCount;i++)
        if(users[i].role==ADMIN) return;
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

// ============================================================
//  REGISTRATION & LOGIN
// ============================================================
void registerUser() {
    if(userCount >= MAX_USERS){ printf("User limit reached.\n"); return; }
    User *u = &users[userCount];
    printf("\n=== PASSENGER REGISTRATION ===\n");
    printf("Name  : "); scanf(" %[^\n]", u->name);
    printf("Phone : "); scanf(" %s", u->phone);
    /* check duplicate phone */
    for(int i=0;i<userCount;i++)
        if(strcmp(users[i].phone, u->phone)==0){ printf("Phone already registered.\n"); return; }
    printf("Password: "); scanf(" %s", u->password);
    u->id       = 2000 + userCount;
    u->role     = PASSENGER;
    u->balance  = 10.0f;   /* welcome credit */
    u->rating   = 5.0f;
    u->totalRides = 0;
    userCount++;
    saveData();
    printf("Registration successful! Welcome credit: $10.00\n");
}

void registerDriver() {
    if(driverCount >= MAX_DRIVERS){ printf("Driver limit reached.\n"); return; }
    Driver *d = &drivers[driverCount];
    printf("\n=== DRIVER REGISTRATION ===\n");
    printf("Name         : "); scanf(" %[^\n]", d->name);
    printf("Phone        : "); scanf(" %s", d->phone);
    for(int i=0;i<driverCount;i++)
        if(strcmp(drivers[i].phone, d->phone)==0){ printf("Phone already registered.\n"); return; }
    printf("Password     : "); scanf(" %s", d->password);
    printf("Vehicle type : "); scanf(" %[^\n]", d->vehicle);
    printf("Plate number : "); scanf(" %s", d->plate);
    d->id       = 3000 + driverCount;
    d->status   = OFFLINE;
    d->rating   = 5.0f;
    d->totalTrips = 0;
    d->earnings = 0;
    /* random starting location (Phnom Penh area ~11.57, 104.92) */
    d->lat = 11.55f + (float)(rand()%30)/100.0f;
    d->lon = 104.88f + (float)(rand()%10)/100.0f;
    driverCount++;
    saveData();
    printf("Driver registration successful!\n");
}

int loginUser(const char *phone, const char *pass) {
    for(int i=0;i<userCount;i++)
        if(strcmp(users[i].phone,phone)==0 && strcmp(users[i].password,pass)==0)
            return i;
    return -1;
}
int loginDriver(const char *phone, const char *pass) {
    for(int i=0;i<driverCount;i++)
        if(strcmp(drivers[i].phone,phone)==0 && strcmp(drivers[i].password,pass)==0)
            return i;
    return -1;
}

// ============================================================
//  BOOKING LOGIC
// ============================================================
int findNearestAvailableDriver(float lat, float lon, RideType rt) {
    int best = -1; float bestDist = 1e9f;
    /* XL requires a specific vehicle keyword */
    for(int i=0;i<driverCount;i++){
        if(drivers[i].status != AVAILABLE) continue;
        if(rt == XL && strstr(drivers[i].vehicle,"XL")==NULL &&
                       strstr(drivers[i].vehicle,"Van")==NULL &&
                       strstr(drivers[i].vehicle,"MPV")==NULL) continue;
        float d = calcDistance(lat, lon, drivers[i].lat, drivers[i].lon);
        if(d < bestDist){ bestDist=d; best=i; }
    }
    return best;
}

void bookRide() {
    if(loggedInUser < 0){ printf("Please log in first.\n"); return; }
    User *u = &users[loggedInUser];
    printf("\n=== BOOK A RIDE ===\n");

    Booking *b = &bookings[bookingCount];
    b->id       = 5000 + bookingCount;
    b->userId   = u->id;
    b->driverId = -1;
    b->userRating   = 0;
    b->driverRating = 0;

    printf("Pickup location  : "); scanf(" %[^\n]", b->pickupLoc);
    printf("Pickup Lat (e.g. 11.56) : "); scanf("%f", &b->pickupLat);
    printf("Pickup Lon (e.g. 104.92): "); scanf("%f", &b->pickupLon);
    printf("Drop location    : "); scanf(" %[^\n]", b->dropLoc);
    printf("Drop   Lat : "); scanf("%f", &b->dropLat);
    printf("Drop   Lon : "); scanf("%f", &b->dropLon);

    printf("Ride type:\n  1. Economy\n  2. Comfort\n  3. XL\nChoice: ");
    int rt; scanf("%d", &rt);
    b->rideType = (rt>=1&&rt<=3) ? (RideType)rt : ECONOMY;

    b->distance = calcDistance(b->pickupLat, b->pickupLon, b->dropLat, b->dropLon);
    b->fare     = calcFare(b->distance, b->rideType);

    printf("\n--- Ride Summary ---\n");
    printf("  Type     : %s\n",   rideTypeName(b->rideType));
    printf("  Distance : %.2f km\n", b->distance);
    printf("  Fare     : $%.2f\n",   b->fare);
    printf("  Balance  : $%.2f\n",   u->balance);

    if(u->balance < b->fare){ printf("Insufficient balance! Please top up.\n"); return; }

    printf("Confirm booking? (1=Yes / 0=No): ");
    int confirm; scanf("%d", &confirm);
    if(!confirm){ printf("Booking cancelled.\n"); return; }

    /* find nearest driver */
    int dIdx = findNearestAvailableDriver(b->pickupLat, b->pickupLon, b->rideType);
    if(dIdx < 0){
        b->status = PENDING;
        b->driverId = -1;
        getTimestamp(b->createdAt);
        bookingCount++;
        saveData();
        printf("No driver available right now. Booking queued (ID: %d).\n", b->id);
        return;
    }

    b->driverId = drivers[dIdx].id;
    b->status   = ACCEPTED;
    drivers[dIdx].status = ON_TRIP;
    u->balance  -= b->fare;
    getTimestamp(b->createdAt);
    bookingCount++;
    saveData();

    float eta = calcDistance(drivers[dIdx].lat, drivers[dIdx].lon, b->pickupLat, b->pickupLon) / 40.0f * 60.0f;
    printf("\nDriver assigned: %s (%s | %s)\n", drivers[dIdx].name, drivers[dIdx].vehicle, drivers[dIdx].plate);
    printf("ETA to pickup  : ~%.0f minutes\n", eta);
    printf("Booking ID     : %d\n", b->id);
}

void viewMyBookings() {
    if(loggedInUser < 0) return;
    int uid = users[loggedInUser].id;
    printf("\n=== MY BOOKINGS ===\n");
    int found = 0;
    for(int i=0;i<bookingCount;i++){
        if(bookings[i].userId != uid) continue;
        found=1;
        Booking *b = &bookings[i];
        printf("--------------------------------------------------\n");
        printf("ID: %d  |  Status: %s  |  %s\n", b->id, bookingStatusName(b->status), b->createdAt);
        printf("From  : %s\nTo    : %s\n", b->pickupLoc, b->dropLoc);
        printf("Type  : %s  |  Distance: %.2f km  |  Fare: $%.2f\n", rideTypeName(b->rideType), b->distance, b->fare);
        if(b->driverId>0){
            for(int d=0;d<driverCount;d++)
                if(drivers[d].id==b->driverId)
                    printf("Driver: %s (%s)\n", drivers[d].name, drivers[d].plate);
        }
        if(b->userRating>0) printf("Your rating: %d/5\n", b->userRating);
    }
    if(!found) printf("No bookings yet.\n");
}

void cancelBooking() {
    if(loggedInUser < 0) return;
    printf("Enter Booking ID to cancel: ");
    int bid; scanf("%d", &bid);
    for(int i=0;i<bookingCount;i++){
        Booking *b = &bookings[i];
        if(b->id==bid && b->userId==users[loggedInUser].id){
            if(b->status==COMPLETED||b->status==CANCELLED){
                printf("Cannot cancel a %s booking.\n", bookingStatusName(b->status)); return;
            }
            b->status = CANCELLED;
            /* refund if accepted */
            if(b->status==ACCEPTED||b->status==IN_PROGRESS){
                users[loggedInUser].balance += b->fare;
                printf("Refund of $%.2f credited.\n", b->fare);
            }
            /* free driver */
            if(b->driverId>0)
                for(int d=0;d<driverCount;d++)
                    if(drivers[d].id==b->driverId) drivers[d].status=AVAILABLE;
            saveData();
            printf("Booking %d cancelled.\n", bid); return;
        }
    }
    printf("Booking not found.\n");
}

void rateDriver() {
    if(loggedInUser < 0) return;
    printf("Enter Booking ID to rate: ");
    int bid; scanf("%d", &bid);
    for(int i=0;i<bookingCount;i++){
        Booking *b = &bookings[i];
        if(b->id==bid && b->userId==users[loggedInUser].id){
            if(b->status!=COMPLETED){ printf("Ride not completed yet.\n"); return; }
            if(b->userRating>0){ printf("Already rated.\n"); return; }
            printf("Rate driver (1-5): ");
            int r; scanf("%d", &r);
            if(r<1||r>5){ printf("Invalid rating.\n"); return; }
            b->userRating = r;
            for(int d=0;d<driverCount;d++)
                if(drivers[d].id==b->driverId){
                    /* running average */
                    drivers[d].rating = (drivers[d].rating * drivers[d].totalTrips + r) / (drivers[d].totalTrips + 1.0f);
                }
            saveData();
            printf("Rating submitted. Thank you!\n"); return;
        }
    }
    printf("Booking not found.\n");
}

void topUpBalance() {
    if(loggedInUser < 0) return;
    printf("Top-up amount ($): ");
    float amt; scanf("%f", &amt);
    if(amt <= 0){ printf("Invalid amount.\n"); return; }
    users[loggedInUser].balance += amt;
    saveData();
    printf("New balance: $%.2f\n", users[loggedInUser].balance);
}

// ============================================================
//  DRIVER ACTIONS
// ============================================================
void driverToggleStatus() {
    Driver *d = &drivers[loggedInDriver];
    if(d->status == ON_TRIP){ printf("Cannot change status while on a trip.\n"); return; }
    d->status = (d->status==AVAILABLE) ? OFFLINE : AVAILABLE;
    saveData();
    printf("Status: %s\n", driverStatusName(d->status));
}

void driverViewTrips() {
    Driver *d = &drivers[loggedInDriver];
    printf("\n=== MY TRIPS ===\n");
    int found=0;
    for(int i=0;i<bookingCount;i++){
        if(bookings[i].driverId != d->id) continue;
        found=1;
        Booking *b = &bookings[i];
        printf("--------------------------------------------------\n");
        printf("ID: %d  |  Status: %s  |  %s\n", b->id, bookingStatusName(b->status), b->createdAt);
        printf("From: %s  ->  To: %s\n", b->pickupLoc, b->dropLoc);
        printf("Fare: $%.2f  |  Distance: %.2f km\n", b->fare, b->distance);
    }
    if(!found) printf("No trips yet.\n");
    printf("\nTotal Earnings: $%.2f | Total Trips: %d | Rating: %.1f\n",
           d->earnings, d->totalTrips, d->rating);
}

void driverCompleteTrip() {
    Driver *d = &drivers[loggedInDriver];
    printf("Enter Booking ID to complete: ");
    int bid; scanf("%d", &bid);
    for(int i=0;i<bookingCount;i++){
        Booking *b = &bookings[i];
        if(b->id==bid && b->driverId==d->id){
            if(b->status==COMPLETED){ printf("Already completed.\n"); return; }
            b->status = COMPLETED;
            d->status = AVAILABLE;
            d->earnings  += b->fare * 0.80f;  /* 80% commission */
            d->totalTrips++;
            /* update driver location to drop point */
            d->lat = b->dropLat;
            d->lon = b->dropLon;
            /* update passenger ride count */
            for(int u=0;u<userCount;u++)
                if(users[u].id==b->userId) users[u].totalRides++;
            saveData();
            printf("Trip completed! Earned: $%.2f\n", b->fare*0.80f); return;
        }
    }
    printf("Booking not found or not yours.\n");
}

void driverAcceptPending() {
    Driver *d = &drivers[loggedInDriver];
    if(d->status != AVAILABLE){ printf("You must be available to accept rides.\n"); return; }
    printf("\n=== PENDING BOOKINGS ===\n");
    int found=0;
    for(int i=0;i<bookingCount;i++){
        Booking *b = &bookings[i];
        if(b->status!=PENDING) continue;
        found=1;
        printf("ID: %d  |  %s -> %s  |  %.2f km  |  $%.2f  |  %s\n",
               b->id, b->pickupLoc, b->dropLoc, b->distance, b->fare, rideTypeName(b->rideType));
    }
    if(!found){ printf("No pending bookings.\n"); return; }
    printf("Enter Booking ID to accept (0=skip): ");
    int bid; scanf("%d", &bid);
    if(bid==0) return;
    for(int i=0;i<bookingCount;i++){
        Booking *b = &bookings[i];
        if(b->id==bid && b->status==PENDING){
            b->driverId = d->id;
            b->status   = ACCEPTED;
            d->status   = ON_TRIP;
            /* deduct fare from passenger */
            for(int u=0;u<userCount;u++)
                if(users[u].id==b->userId){ users[u].balance -= b->fare; break; }
            saveData();
            printf("Booking accepted! Heading to %s\n", b->pickupLoc); return;
        }
    }
    printf("Booking not found or already taken.\n");
}

// ============================================================
//  ADMIN PANEL
// ============================================================
void adminViewAllUsers() {
    printf("\n%-5s %-20s %-15s %-10s %-8s %-6s\n","ID","Name","Phone","Role","Balance","Rides");
    printf("----------------------------------------------------------------------\n");
    for(int i=0;i<userCount;i++){
        User *u=&users[i];
        const char *role = u->role==ADMIN?"Admin":u->role==DRIVER?"Driver":"Passenger";
        printf("%-5d %-20s %-15s %-10s $%-7.2f %-6d\n",
               u->id, u->name, u->phone, role, u->balance, u->totalRides);
    }
}

void adminViewAllDrivers() {
    printf("\n%-5s %-20s %-12s %-20s %-8s %-7s %-6s\n","ID","Name","Phone","Vehicle","Status","Earn","Trips");
    printf("------------------------------------------------------------------------------\n");
    for(int i=0;i<driverCount;i++){
        Driver *d=&drivers[i];
        printf("%-5d %-20s %-12s %-20s %-8s $%-6.2f %-6d\n",
               d->id, d->name, d->phone, d->vehicle,
               driverStatusName(d->status), d->earnings, d->totalTrips);
    }
}

void adminViewAllBookings() {
    printf("\n%-6s %-6s %-6s %-10s %-12s %-6s %-10s\n","BID","UID","DID","Status","Type","km","Fare");
    printf("----------------------------------------------------------------\n");
    for(int i=0;i<bookingCount;i++){
        Booking *b=&bookings[i];
        printf("%-6d %-6d %-6d %-10s %-12s %-6.1f $%.2f\n",
               b->id, b->userId, b->driverId,
               bookingStatusName(b->status), rideTypeName(b->rideType),
               b->distance, b->fare);
    }
}

void adminStats() {
    float totalRevenue=0, totalDriverPay=0;
    int completed=0, cancelled=0, pending=0;
    for(int i=0;i<bookingCount;i++){
        switch(bookings[i].status){
            case COMPLETED: totalRevenue+=bookings[i].fare; completed++; break;
            case CANCELLED: cancelled++; break;
            case PENDING:   pending++;   break;
            default: break;
        }
    }
    for(int i=0;i<driverCount;i++) totalDriverPay+=drivers[i].earnings;
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

void adminDeleteUser() {
    printf("Enter User ID to delete: ");
    int uid; scanf("%d",&uid);
    for(int i=0;i<userCount;i++){
        if(users[i].id==uid){
            if(users[i].role==ADMIN){printf("Cannot delete admin.\n");return;}
            for(int j=i;j<userCount-1;j++) users[j]=users[j+1];
            userCount--;
            saveData();
            printf("User deleted.\n"); return;
        }
    }
    printf("User not found.\n");
}

// ============================================================
//  MENUS
// ============================================================
void passengerMenu() {
    int choice;
    do {
        User *u = &users[loggedInUser];
        clearScreen();
        printf("╔══════════════════════════════╗\n");
        printf("║   RIDE APP — PASSENGER MENU  ║\n");
        printf("╚══════════════════════════════╝\n");
        printf("  Welcome, %s  |  Balance: $%.2f\n\n", u->name, u->balance);
        printf("  1. Book a Ride\n");
        printf("  2. My Bookings\n");
        printf("  3. Cancel Booking\n");
        printf("  4. Rate Driver\n");
        printf("  5. Top Up Balance\n");
        printf("  0. Logout\n");
        printf("\nChoice: "); scanf("%d",&choice);
        switch(choice){
            case 1: bookRide();       pauseOnly(); break;
            case 2: viewMyBookings(); pauseOnly(); break;
            case 3: cancelBooking();  pauseOnly(); break;
            case 4: rateDriver();     pauseOnly(); break;
            case 5: topUpBalance();   pauseOnly(); break;
            case 0: loggedInUser=-1; printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while(choice!=0);
}

void driverMenu() {
    int choice;
    do {
        Driver *d = &drivers[loggedInDriver];
        clearScreen();
        printf("╔══════════════════════════════╗\n");
        printf("║   RIDE APP — DRIVER MENU     ║\n");
        printf("╚══════════════════════════════╝\n");
        printf("  %s  |  %s  |  Rating: %.1f\n\n", d->name, driverStatusName(d->status), d->rating);
        printf("  1. Toggle Online/Offline\n");
        printf("  2. View Pending Bookings & Accept\n");
        printf("  3. Complete Current Trip\n");
        printf("  4. My Trip History & Earnings\n");
        printf("  0. Logout\n");
        printf("\nChoice: "); scanf("%d",&choice);
        switch(choice){
            case 1: driverToggleStatus();   pauseOnly(); break;
            case 2: driverAcceptPending();  pauseOnly(); break;
            case 3: driverCompleteTrip();   pauseOnly(); break;
            case 4: driverViewTrips();      pauseOnly(); break;
            case 0: loggedInDriver=-1; printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while(choice!=0);
}

void adminMenu() {
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
        printf("\nChoice: "); scanf("%d",&choice);
        switch(choice){
            case 1: adminViewAllUsers();    pauseOnly(); break;
            case 2: adminViewAllDrivers();  pauseOnly(); break;
            case 3: adminViewAllBookings(); pauseOnly(); break;
            case 4: adminStats();           pauseOnly(); break;
            case 5: adminDeleteUser();      pauseOnly(); break;
            case 0: isAdmin=0; printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while(choice!=0);
}

// ============================================================
//  MAIN LOGIN / REGISTER MENU
// ============================================================
void loginMenu() {
    char phone[MAX_PHONE], pass[MAX_PASS];
    int choice;
    printf("\n=== LOGIN ===\n1. Passenger  2. Driver  3. Admin\nChoice: ");
    scanf("%d",&choice);
    printf("Phone   : "); scanf(" %s", phone);
    printf("Password: "); scanf(" %s", pass);

    if(choice==1){
        int idx = loginUser(phone, pass);
        if(idx<0){ printf("Invalid credentials.\n"); return; }
        if(users[idx].role==ADMIN){ isAdmin=1; passengerMenu(); return; }
        loggedInUser = idx;
        passengerMenu();
    } else if(choice==2){
        int idx = loginDriver(phone, pass);
        if(idx<0){ printf("Invalid credentials.\n"); return; }
        loggedInDriver = idx;
        driverMenu();
    } else if(choice==3){
        int idx = loginUser(phone, pass);
        if(idx<0||users[idx].role!=ADMIN){ printf("Invalid admin credentials.\n"); return; }
        isAdmin=1;
        adminMenu();
    } else {
        printf("Invalid option.\n");
    }
}

// ============================================================
//  MAIN
// ============================================================
int main() {
    srand((unsigned)time(NULL));
    loadData();
    seedAdminIfNeeded();

    int choice;
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   RIDE MANAGEMENT SYSTEM v1.0            ║\n");
    printf("║   Similar to Grab / WowNow               ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    do {
        printf("\n=== MAIN MENU ===\n");
        printf("  1. Login\n");
        printf("  2. Register as Passenger\n");
        printf("  3. Register as Driver\n");
        printf("  0. Exit\n");
        printf("Choice: "); scanf("%d",&choice);
        switch(choice){
            case 1: loginMenu();      break;
            case 2: registerUser();   pauseOnly(); break;
            case 3: registerDriver(); pauseOnly(); break;
            case 0: printf("Goodbye!\n"); saveData(); break;
            default: printf("Invalid choice.\n");
        }
    } while(choice!=0);

    return 0;
}