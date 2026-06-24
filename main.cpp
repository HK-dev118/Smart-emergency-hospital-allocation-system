#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_AREAS 10
#define MAX_PATIENTS 100
#define MAX_HOSPITALS 10

// ===================== PATIENT =====================
typedef struct {
    int id;
    int severity;
    int location;
    int assignedHospital;
    int arrivalTime;
    int waitTime;
} Patient;

// ===================== HOSPITAL =====================
typedef struct {
    int id;
    int totalBeds;
    int availableBeds;
    int availableVentilators;
    int occupiedBeds;
    int totalDoctors;
    int availableDoctors;
} Hospital;

// ===================== GLOBALS =====================
Patient heap[MAX_PATIENTS];
int heapSize = 0;

Patient waitingHeap[MAX_PATIENTS];
int waitingSize = 0;

Patient admitted[MAX_PATIENTS];
int admittedCount = 0;

Hospital hospitals[MAX_HOSPITALS];
int hospitalCount = 0;

int graph[MAX_HOSPITALS][MAX_HOSPITALS];
int currentTime = 0;

int totalAssigned = 0;
int totalWaitTime = 0;

// ===================== UTIL =====================
void swap(Patient *a, Patient *b){
    Patient t=*a; *a=*b; *b=t;
}

int priorityValue(Patient p){
    return p.severity*200 + p.waitTime*10;
}

int compare(Patient a, Patient b){
    int pa=priorityValue(a), pb=priorityValue(b);
    if(pa==pb) return a.arrivalTime < b.arrivalTime;
    return pa>pb;
}

// ===================== HEAP (MAIN) =====================
void heapifyUp(int i){
    while(i>0){
        int p=(i-1)/2;
        if(compare(heap[i],heap[p])){
            swap(&heap[i],&heap[p]);
            i=p;
        } else break;
    }
}

void heapifyDown(int i){
    int best=i,l=2*i+1,r=2*i+2;

    if(l<heapSize && compare(heap[l],heap[best])) best=l;
    if(r<heapSize && compare(heap[r],heap[best])) best=r;

    if(best!=i){
        swap(&heap[i],&heap[best]);
        heapifyDown(best);
    }
}

// ===================== WAITING HEAP =====================
void wUp(int i){
    while(i>0){
        int p=(i-1)/2;
        if(compare(waitingHeap[i],waitingHeap[p])){
            swap(&waitingHeap[i],&waitingHeap[p]);
            i=p;
        } else break;
    }
}

void wDown(int i){
    int best=i,l=2*i+1,r=2*i+2;

    if(l<waitingSize && compare(waitingHeap[l],waitingHeap[best])) best=l;
    if(r<waitingSize && compare(waitingHeap[r],waitingHeap[best])) best=r;

    if(best!=i){
        swap(&waitingHeap[i],&waitingHeap[best]);
        wDown(best);
    }
}

// ===================== GRAPH =====================
void initGraph(){
    for(int i=0;i<MAX_HOSPITALS;i++)
        for(int j=0;j<MAX_HOSPITALS;j++)
            graph[i][j]=(i==j)?0:INT_MAX;
}

void addRoad(int u,int v,int w){
    graph[u][v]=graph[v][u]=w;
}

// ===================== DIJKSTRA =====================
void dijkstra(int src,int dist[]){
    int vis[MAX_HOSPITALS]={0};

    for(int i=0;i<hospitalCount;i++)
        dist[i]=INT_MAX;

    dist[src]=0;

    for(int i=0;i<hospitalCount;i++){
        int u=-1,m=INT_MAX;

        for(int j=0;j<hospitalCount;j++)
            if(!vis[j] && dist[j]<m)
                m=dist[j],u=j;

        if(u==-1) break;

        vis[u]=1;

        for(int v=0;v<hospitalCount;v++){
            if(!vis[v] && graph[u][v]!=INT_MAX &&
               dist[u]+graph[u][v]<dist[v]){
                dist[v]=dist[u]+graph[u][v];
            }
        }
    }
}

// ===================== BEST HOSPITAL =====================
int findBestHospital(int area){
    int dist[MAX_HOSPITALS];
    dijkstra(area,dist);

    int best=-1;
    double bestScore=1e9;

    for(int i=0;i<hospitalCount;i++){

        if(dist[i]==INT_MAX) continue;

        double load=(double)hospitals[i].occupiedBeds /
                    (double)hospitals[i].totalBeds;

        double score=dist[i]+load*100;

        if(score<bestScore){
            bestScore=score;
            best=i;
        }
    }
    return best;
}

// ===================== EVICITON =====================
int evictLowPriority(int h){

    int idx=-1;
    int minP=INT_MAX;

    for(int i=0;i<admittedCount;i++){
        if(admitted[i].assignedHospital==h){
            int p=priorityValue(admitted[i]);
            if(p<minP){
                minP=p;
                idx=i;
            }
        }
    }

    if(idx==-1) return 0;

    printf("Evicting patient %d for emergency\n",admitted[idx].id);

    hospitals[h].occupiedBeds--;
    hospitals[h].availableBeds++;
    hospitals[h].availableDoctors++;

    admitted[idx]=admitted[--admittedCount];

    return 1;
}

// ===================== MOVE WAITING =====================
void moveWaiting(){

    while(waitingSize>0 && heapSize<MAX_PATIENTS){

        Patient p=waitingHeap[0];

        waitingHeap[0]=waitingHeap[--waitingSize];
        wDown(0);

        heap[heapSize]=p;
        heapifyUp(heapSize++);
    }
}

// ===================== ADD PATIENT =====================
void addPatient(int id,int sev,int loc){

    if(heapSize>=MAX_PATIENTS){
        if(waitingSize<MAX_PATIENTS){
            waitingHeap[waitingSize]=(Patient){id,sev,loc,-1,currentTime,0};
            wUp(waitingSize++);
        }
        return;
    }

    heap[heapSize]=(Patient){id,sev,loc,-1,currentTime,0};
    heapifyUp(heapSize++);
}

// ===================== POP =====================
Patient popPatient(){
    if(heapSize==0)
        return (Patient){-1,-1,-1,-1,-1,-1};

    Patient root=heap[0];
    heap[0]=heap[--heapSize];
    heapifyDown(0);
    return root;
}

// ===================== HOSPITAL =====================
void addHospital(int id,int b,int v,int d){
    if(hospitalCount<MAX_HOSPITALS)
        hospitals[hospitalCount++]=(Hospital){id,b,b,v,0,d,d};
}

// ===================== ALLOCATION =====================
void allocatePatient(){

    if(heapSize==0) return;

    Patient p=popPatient();

    int h=findBestHospital(p.location);

    if(h==-1){
        waitingHeap[waitingSize]=p;
        wUp(waitingSize++);
        return;
    }

    if(hospitals[h].availableBeds<=0 ||
       hospitals[h].availableDoctors<=0){

        if(p.severity==3){

            if(!evictLowPriority(h)){

                for(int i=0;i<hospitalCount;i++){
                    if(i!=h){
                        h=i;
                        break;
                    }
                }
            }

        } else {
            waitingHeap[waitingSize]=p;
            wUp(waitingSize++);
            return;
        }
    }

    hospitals[h].availableBeds--;
    hospitals[h].occupiedBeds++;
    hospitals[h].availableDoctors--;

    p.assignedHospital=h;
    admitted[admittedCount++]=p;

    totalAssigned++;
    totalWaitTime += (currentTime - p.arrivalTime);

    printf("Patient %d assigned to Hospital %d\n",p.id,h);

    moveWaiting();
}
void dischargePatient(int hospitalId){

    int found = 0;

    for(int i=0;i<admittedCount;i++){

        if(admitted[i].assignedHospital == hospitalId){

            // free hospital resources
            hospitals[hospitalId].occupiedBeds--;
            hospitals[hospitalId].availableBeds++;
            hospitals[hospitalId].availableDoctors++;

            // remove from admitted[]
            admitted[i] = admitted[--admittedCount];

            printf("Patient discharged from Hospital %d\n", hospitalId);

            found = 1;
            break;
        }
    }

    if(!found){
        printf("No patient found in Hospital %d\n", hospitalId);
    }

    // system recovery
    moveWaiting();
}
void showHospitals(){

    printf("\n===== HOSPITAL STATUS =====\n");

    for(int i=0;i<hospitalCount;i++){

        printf("\nHospital ID: %d\n", hospitals[i].id);
        printf("Total Beds: %d\n", hospitals[i].totalBeds);
        printf("Available Beds: %d\n", hospitals[i].availableBeds);
        printf("Occupied Beds: %d\n", hospitals[i].occupiedBeds);
        printf("Doctors: %d/%d\n",
               hospitals[i].availableDoctors,
               hospitals[i].totalDoctors);
        printf("Ventilators: %d\n", hospitals[i].availableVentilators);
    }
}
void showWaitingPatients(){

    printf("\n===== WAITING PATIENTS =====\n");

    if(waitingSize == 0){
        printf("No waiting patients\n");
        return;
    }

    for(int i=0;i<waitingSize;i++){

        printf("\nPatient ID   : %d\n", waitingHeap[i].id);
        printf("Severity     : %d\n", waitingHeap[i].severity);
        printf("Location     : %d\n", waitingHeap[i].location);
        printf("Arrival Time : %d\n", waitingHeap[i].arrivalTime);
        printf("Wait Time    : %d hours\n",
               currentTime - waitingHeap[i].arrivalTime);
    }
}
void showMetrics(){

    printf("\n===== REAL-TIME SYSTEM METRICS =====\n");

    // ACTIVE PATIENTS ONLY
    printf("Currently Admitted Patients: %d\n", admittedCount);
    printf("Patients in Waiting Queue   : %d\n", waitingSize);
    printf("Patients in Heap (Pending)  : %d\n", heapSize);

    // HOSPITAL STATUS
    printf("\n--- Hospital Load ---\n");

    for(int i=0;i<hospitalCount;i++){

        double load = (double)hospitals[i].occupiedBeds /
                      (double)hospitals[i].totalBeds;

        printf("\nHospital %d\n", hospitals[i].id);
        printf("Beds: %d/%d\n",
               hospitals[i].occupiedBeds,
               hospitals[i].totalBeds);

        printf("Doctors: %d/%d\n",
               hospitals[i].availableDoctors,
               hospitals[i].totalDoctors);

        printf("Load: %.2f\n", load);
    }

    // PERFORMANCE METRIC (only active patients)
    if(admittedCount > 0){
        int totalWait = 0;

        for(int i=0;i<admittedCount;i++){
            totalWait += (currentTime - admitted[i].arrivalTime);
        }

        printf("\nAverage Waiting Time (Active Patients): %.2f hours\n",
               (double)totalWait / admittedCount);
    } else {
        printf("\nNo active patients currently\n");
    }
}
// ===================== MAIN =====================
int main() {
    initGraph();
    addHospital(0, 1, 0,1);
    addHospital(1, 0, 0,0);
    addHospital(2, 0, 0,0);
    addRoad(0, 1, 10);
    addRoad(1, 2, 5);
    addRoad(0, 2, 15);
    int choice;
    while (1) {
        currentTime++;
        printf("\n===== SYSTEM MENU =====\n");
        printf("1 Add Patient\n");
        printf("2 Allocate Patient\n");
        printf("3 Show Hospitals\n");
        printf("4 Discharge Patient\n");
        printf("5 Show Metrics\n");
        printf("6 Show Waiting Patients\n");
        printf("7 Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 1) {
            int n;
            printf("How many patients: ");
            scanf("%d", &n);
            for (int i = 0; i < n; i++) {
                int id, sev, loc;
                printf("Patient %d (ID Sev Loc): ", i + 1);
                scanf("%d %d %d", &id, &sev, &loc);
                addPatient(id, sev, loc);
            }
        }

        else if (choice == 2) {
            while (heapSize > 0) {
                allocatePatient();
            }
        }
        else if (choice == 3) {
            showHospitals();
        }

        else if (choice == 4) {
            int id;
            printf("Hospital ID: ");
            scanf("%d", &id);
            dischargePatient(id);
        }

        else if (choice == 5) {
            showMetrics();
        }
        else if (choice == 6) {
            showWaitingPatients();
        }
        else if (choice == 7) {
            break;
        }
        else break;
    }

    return 0;
}
