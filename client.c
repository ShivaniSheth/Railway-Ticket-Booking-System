#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>

#define PORT 8090
#define PASS_LENGTH 20
#define BOOKING "./db/booking"
#define TRAIN "./db/train"
#define USER_CUSTOMER "./db/accounts/customer"
#define USER_ADMIN "./db/accounts/admin"
#define USER_AGENT "./db/accounts/agent"

struct account
{
	int id;
	char name[10];
	char pass[PASS_LENGTH];
};

struct bookings
{
	int bid;
	int type;
	int acc_no;
	int tr_id;
	char trainname[20];
	int seat_start;
	int seat_end;
	int cancelled;
};

struct train
{
	int tid;
	char train_name[20];
	int train_no;
	int av_seats;
	int last_seatno_used;
};


int entry_point(int sock);
int display(int sock, int type);
int display_admin(int sock, int action);
int display_users(int sock, int choice_);
void view_booking(int sock);


int entry_point(int sock){
	int choice_;
	system("clear");
	printf("1. Sign In\n");
	printf("2. Sign Up\n");
	printf("3. Exit\n");
	printf("Enter Your Choice\n");
	scanf("%d", &choice_);

	while(choice_ > 3 || choice_ < 1)
	{
		printf("Invalid Choice!\n");
		printf("Enter Your Choice\n");
		scanf("%d", &choice_);
	}

	write(sock, &choice_, sizeof(choice_));
	if(choice_==1){
		int type, acc_no;
		char password[PASS_LENGTH];
		printf("Enter the type of account:\n");
		printf("1.Customer\n2.Agent\n3.Admin\n4.Back\n");
		printf("Your Choice: ");
		scanf("%d", &type);

		while(type > 4 || type < 1)
		{
			printf("Invalid Choice!\n");
			printf("Your Choice: ");
			scanf("%d", &type);
		}

		if(type == 4)
		{
			//go back to main menu
			//returning !=3 will result in calling entry_point() again
			write(sock,&type,sizeof(type));
			return 0;
		}

		printf("Enter Your Account Number: ");
		scanf("%d", &acc_no);
		strcpy(password,getpass("Enter password: "));

		write(sock, &type, sizeof(type));
		write(sock, &acc_no, sizeof(acc_no));
		write(sock, &password, strlen(password));

		int valid_login;
		read(sock, &valid_login, sizeof(valid_login));
		if(valid_login == 1){
			while (display(sock, type)!=-1);
			system("clear");
			return 1;
		}
		else{
			printf("Login Failed\n");
			while(getchar()!='\n');
			getchar();
			return 1;
		}
	}
	else if(choice_==2){
		int type, acc_no;
		char password[PASS_LENGTH], name[10];
		printf("Enter the type of account :\n");
		printf("1.Customer\n2.Agent\n3.Admin\n4.Back\n");
		printf("Your Choice: ");
		scanf("%d", &type);

		while(type > 4 || type < 1)
		{
			printf("Invalid Choice!\n");
			printf("Your Choice : ");
			scanf("%d", &type);
		}

		if(type == 4)
		{
			write(sock, &type, sizeof(type));
			return 0;
		}
		printf("Enter your name : ");scanf("%s", name);
		strcpy(password,getpass("Enter password: "));
		
		write(sock, &type, sizeof(type));
		write(sock, &name, sizeof(name));
		write(sock, &password, strlen(password));

		read(sock, &acc_no, sizeof(acc_no));
		printf("Your account No. is : %d.\nThe account no of further login\n", acc_no);
		while(getchar()!='\n');
		getchar();
		return 2;
	}
	else if(choice_ == 3)
	{	return 3;}
		
}

int display(int sock, int type){
	int choice_ = 0;
	if(type == 1 || type == 2){
		system("clear");		
		printf("1. Book Ticket\n2. View Bookings\n3. Update Booking\n4. Cancel booking\n5. Logout\n");
		printf("Your Choice: ");
		scanf("%d", &choice_);
		return display_users(sock, choice_);
		return -1;
	}
	else{
		system("clear");
		printf("1. Add Train\n2. Delete Train\n3. Modify Train\n4. Display users\n5. Delete User\n6. Logout\n7. View Booking'\n8. View Train\n");	
		printf("Your Choice: ");
		scanf("%d", &choice_);
		return display_admin(sock, choice_);
	}
}

int display_admin(int sock, int choice_){
	int fd,fp;
	switch(choice_)
	{
		case 1:
		{
			int tno;
			char tname[20];
			write(sock, &choice_, sizeof(choice_));
			printf("Enter Train Name: ");scanf("%s", tname);
			printf("Enter Train No. : ");scanf("%d", &tno);
			write(sock, &tname, sizeof(tname));
			write(sock, &tno, sizeof(tno));
			read(sock, &choice_, sizeof(choice_));
			if(choice_ == 1 ) printf("Train Added Successfully.\n");
			while(getchar()!='\n');
			getchar();
			return choice_;
			break;
		}
		case 2:
		{
			int no_of_trains;
			write(sock, &choice_, sizeof(choice_));
			read(sock, &no_of_trains, sizeof(int));
			while(no_of_trains>0){
				int tid, tno;
				char tname[20];
				read(sock, &tid, sizeof(tid));
				read(sock, &tname, sizeof(tname));
				read(sock, &tno, sizeof(tno));
				if(!strcmp(tname, "deleted"));else
				printf("%d.\t%d\t%s\n", tid+1, tno, tname);
				no_of_trains--;
			}
			printf("Enter 0 to cancel.\nEnter the train ID to delete: "); scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			read(sock, &choice_, sizeof(choice_));
			
			if(choice_ == 0)printf("Operation aborted");	//cancel choice_ion chose
			else printf("Train deleted successfully\n");	//other than cancel
			
			while(getchar()!='\n');
			getchar();
			return choice_;
			break;
		}
		case 3:
		{
			int no_of_trains,is_valid;
			write(sock, &choice_, sizeof(choice_));
			read(sock, &no_of_trains, sizeof(int));
			while(no_of_trains>0){
				int tid, tno;
				char tname[20];
				read(sock, &tid, sizeof(tid));
				read(sock, &tname, sizeof(tname));
				read(sock, &tno, sizeof(tno));
				if(!strcmp(tname, "deleted"));else
				printf("%d.\t%d\t%s\n", tid+1, tno, tname);
				no_of_trains--;
			}
			printf("Enter 0 to cancel.\nEnter the train ID to modify: "); scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			read(sock,&is_valid,sizeof(is_valid));
			if(is_valid == 0){
				printf("Operation aborted!\n");
			}
			else if(is_valid != 0){
				printf("Which parameter do you want to modify?\n1. Train Name\n2. Available Seats\n");
				printf("Your Choice: ");scanf("%d", &no_of_trains);
				write(sock, &no_of_trains, sizeof(int));
				// if(no_of_trains == 2 || no_of_trains == 3){
				if(no_of_trains == 2 )
				{

					read(sock, &no_of_trains, sizeof(int));
					printf("Current Value: %d\n", no_of_trains);				
					printf("Enter Value: ");scanf("%d", &no_of_trains);
					write(sock, &no_of_trains, sizeof(int));
				}
				else if(no_of_trains == 1){
					char name[20];
					read(sock, &name, sizeof(name));
					printf("Current Value: %s\n", name);
					printf("Enter Value: ");scanf("%s", name);
					write(sock, &name, sizeof(name));
				}
				read(sock, &choice_, sizeof(choice_));
				if(choice_ == no_of_trains) printf("Train Data Modified Successfully\n"); //if modified
			}
			while(getchar()!='\n');
			getchar();
			return choice_;
			break;
		}
		case 4:
		{
		printf("1. Customer\n");
        printf("2. Agents\n");
        printf("3. Admins\n");
        printf("Enter Your Choice\n");
        int opt2;
        struct account acc;
        scanf("%d", &opt2);
        printf("ID\t\tName\n");
        switch (opt2)
        {
        case 1:
            fd = open(USER_CUSTOMER,O_RDONLY);
            fp = lseek(fd,0,SEEK_SET);
            while(read(fd,&acc,sizeof(acc)))
                printf("%d\t\t%s\n",acc.id,acc.name);
            close(fd);
            break;
        case 2:
            fd = open(USER_AGENT,O_RDONLY);
            fp = lseek(fd,0,SEEK_SET);
            while(read(fd,&acc,sizeof(acc)))
                printf("%d\t\t%s\n",acc.id,acc.name);
            close(fd);
            break;
        case 3:
            fd = open(USER_ADMIN,O_RDONLY);
            fp = lseek(fd,0,SEEK_SET);
            while(read(fd,&acc,sizeof(acc)))
                printf("%d\t\t%s\n",acc.id,acc.name);
            close(fd);
            break;
        	default:
            printf("Invalid Choice\n");
            break;
        }  
        while(getchar()!='\n');
			getchar();
			return choice_;
        break;      
		}
		case 5: 
		{
			int choice, users, id;
			write(sock, &choice_, sizeof(choice_));
			printf("What kind of account do you want to delete?\n");
			printf("1. Customer\n2. Agent\n3. Admin\n");
			printf("Your Choice: ");
			scanf("%d", &choice);
			while(choice <1 || choice >3)
			{
				printf("Invalid Choice!\n");
				printf("1. Customer\n2. Agent\n3. Admin\n");
				printf("Your Choice: ");
				scanf("%d", &choice);
			}
			write(sock, &choice, sizeof(choice));
			read(sock, &users, sizeof(users));
			while(users--)
			{
				char name[10];
				read(sock, &id, sizeof(id));
				read(sock, &name, sizeof(name));
				if(strcmp(name, "deleted")!=0)
				printf("%d\t%s\n", id, name);
			}
			printf("Enter 0 to cancel\nEnter the ID to delete: ");scanf("%d", &id);
			write(sock, &id, sizeof(id));
			read(sock, &choice_, sizeof(choice_));
			if(choice_ == 0)
			{
				printf("Operation aborted!\n");
			} 
			else{
				printf("Successfully deleted user\n");
			}
			while(getchar()!='\n');
			getchar();
			return choice_;
		}
		case 6: 
		{
			write(sock, &choice_, sizeof(choice_));
			read(sock, &choice_, sizeof(choice_));
			if(choice_==6) printf("Logged out successfully.\n");
			while(getchar()!='\n');
			getchar();
			return -1;
			break;
		}
		case 7:
		{
			//search users
			printf("\nhey 7\n");
			fd = open(BOOKING,O_RDONLY);
        	fp = lseek(fd,0,SEEK_SET);
        	struct bookings book;
        	printf("BookingID\tUserID\tType\t1st Ticket\tLast Ticket\tTrainId\tTrainName\tisCancelled\n");
        	while(read(fd,&book,sizeof(book)))
            printf("%d\t\t%d\t%d\t%d\t\t%d\t\t%d\t%s\t\t%d\n",book.bid,book.acc_no,book.type,book.seat_start,book.seat_end,book.tr_id,book.trainname,book.cancelled);
        	close(fd);
        	while(getchar()!='\n');
			getchar();
			return choice_;
        	break;		
        }
		case 8:
		{
			printf("hey 8\n");
			fd = open(TRAIN,O_RDONLY);
        	fp = lseek(fd,0,SEEK_SET);
        	struct train tr;
        	printf("ID\tT_NO\tAV_SEAT\tTRAIN NAME\tLastSeatNoUsed\n");
        	while(read(fd,&tr,sizeof(tr)))
            printf("%d\t%d\t%d\t%s\t\t%d\n",tr.tid,tr.train_no,tr.av_seats,tr.train_name,tr.last_seatno_used);
        	close(fd);
        	while(getchar()!='\n');
			getchar();
			return choice_;
        	break;
		}
		default: 
		return -1;
	}
}

int display_users(int sock, int choice_){
	write(sock, &choice_, sizeof(choice_));
	switch(choice_){
		case 1:{
			//book a ticket
			int trains, trainid, trainavseats, trainno, required_seats,last_seatno_used,lastTrainId,valid = 0;
			int t_cnt;
			char trainname[20];
			read(sock, &trains, sizeof(trains));
			// struct train av_trains[trains];
			printf("ID\tT_NO\tAV_SEAT\tTRAIN NAME\n");
			
			// t_cnt=0;
			while(trains--){
				read(sock, &trainid, sizeof(trainid));
				read(sock, &trainno, sizeof(trainno));
				read(sock, &trainavseats, sizeof(trainavseats));
				read(sock, &trainname, sizeof(trainname));
				read(sock, &last_seatno_used, sizeof(last_seatno_used));


				if(strcmp(trainname, "deleted")!=0){
					printf("%d\t%d\t%d\t%s\n", trainid+1, trainno, trainavseats, trainname);
				}
			}
			lastTrainId = trainid;
			printf("Enter the train ID: "); scanf("%d", &trainid);
			
			write(sock, &trainid, sizeof(trainid));
			read(sock,&valid,sizeof(valid));

			if(valid){

				read(sock, &trainavseats, sizeof(trainavseats));
				printf("Available seats : %d\n",trainavseats);
				printf("Enter the number of seats: "); scanf("%d", &required_seats);
				if(trainavseats>=required_seats && required_seats>0)
					write(sock, &required_seats, sizeof(required_seats));
				else{
					required_seats = -1;
					write(sock, &required_seats, sizeof(required_seats));
				}
				read(sock, &choice_, sizeof(choice_));
				
				if(choice_ == 1) printf("Tickets booked successfully\n");
				else printf("Tickets were not booked. Please try again.\n");
			}
			else{
				printf("Operation Aborded\n");
			}
			printf("Press any key to continue...\n");
			while(getchar()!='\n');
			getchar();
			while(!getchar());
			return 1;
		}
		case 2:{
			//View your bookings
			view_booking(sock);
			read(sock, &choice_, sizeof(choice_));
			return 2;
		}
		case 3:{
			//update bookings
			int val;
			view_booking(sock);
			printf("Enter the booking id to be updated: "); scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Booking ID
			printf("What information do you want to update:\n1.Increase No of Seats\n2. Decrease No of Seats\nYour Choice: ");
			scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Increase or Decrease
			if(val == 1){
				printf("How many tickets do you want to increase: ");scanf("%d",&val);
				write(sock, &val, sizeof(int));	//No of Seats
			}else if(val == 2){
				printf("How many tickets do you want to decrease: ");scanf("%d",&val);
				write(sock, &val, sizeof(int));	//No of Seats		
			}
			read(sock, &choice_, sizeof(choice_));
			if(choice_ == -2)
				printf("Operation failed. No more available seats\n");
			else printf("Operation succeded.\n");
			while(getchar()!='\n');
			getchar();
			return 3;
		}
		case 4:{
			//cancel booking
			int bid;
			view_booking(sock);
			printf("Enter the booking id to be Cancelled: "); scanf("%d", &bid);
			write(sock, &bid, sizeof(int));	//Booking ID
			read(sock,&choice_,sizeof(int));
			if(choice_){
				printf("Cancelled Succesfully\n");
			}
			else{
				printf("Cancellation failed!\n");
			}
			getchar();
			getchar();
			return 4;
		} 
		case 5: {
			
			read(sock, &choice_, sizeof(choice_));
			if(choice_ == 5) printf("Logged out successfully.\n");
			while(getchar()!='\n');
			getchar();
			return -1;
			break;
		}
		default: return -1;
	}
}

void view_booking(int sock)
{
	int entries;
	read(sock, &entries, sizeof(int));
	if(entries == 0) printf("No records found.\n");
	else printf("Your recent %d bookings are :\n", entries);
	while(!getchar());
	while(entries--)
	{
		int bid, bks_seat, bke_seat, cancelled,tno;
		// char trainname[20];
		read(sock,&bid, sizeof(bid));
		read(sock,&tno, sizeof(int));
		read(sock,&bks_seat, sizeof(int));
		read(sock,&bke_seat, sizeof(int));
		read(sock,&cancelled, sizeof(int));
		if(!cancelled)
		printf("BookingID: %d\t1st Ticket: %d\tLast Ticket: %d\tTrainNo. :%d\n", bid+1, bks_seat, bke_seat, tno);
	}
	printf("Press any key to continue...\n");
	while(getchar()!='\n');
	getchar();
}

int main(int argc, char * argv[]){
	char *ip = "127.0.0.1";
	if(argc==2){
		ip = argv[1];
	}
	int cli_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(cli_fd == -1){
		printf("socket creation failed\n");
		exit(0);
	}
	struct sockaddr_in ca;
	ca.sin_family=AF_INET;
	ca.sin_port= htons(PORT);
	ca.sin_addr.s_addr = inet_addr(ip);
	if(connect(cli_fd, (struct sockaddr *)&ca, sizeof(ca))==-1){
		printf("connect failed\n");
		exit(0);
	}
	printf("connection established\n");
	
	while(entry_point(cli_fd)!=3);
	close(cli_fd);

	return 0;
}
