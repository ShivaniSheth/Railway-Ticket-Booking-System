#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#define PORT 8090
#define PASS_LENGTH 20
#define TRAIN "./db/train"
#define BOOKING "./db/booking"
#define ERR_EXIT(msg) do{perror(msg);exit(EXIT_FAILURE);}while(0)

struct account
{
	int id;
	char name[10];
	char pass[PASS_LENGTH];
};

struct train
{
	int tid;
	char train_name[20];
	int train_no;
	int av_seats;
	int last_seatno_used;
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

char *ACCOUNT[3] = {"./db/accounts/customer", "./db/accounts/agent", "./db/accounts/admin"};

void service_cli(int sock);
int login(int sock);
int signup(int sock);
int display_menu_admin(int sock, int id);
int menu1(int sock, int id, int type);
void view_booking(int sock, int id, int type);
void view_booking2(int sock, int id, int type, int fd);


void service_cli(int sock){
	int func_id;
	printf("Client [%d] connected\n", sock);
	while(1){		
		printf("Reading option\n");
		read(sock, &func_id, sizeof(int));
		// recv(sock,&func_id, sizeof(int),MSG_WAITALL);
		printf("Read %d\n",func_id);
		if(func_id==1) {login(sock);}
		else if(func_id==2) {signup(sock);}
		// if(func_id==3) break;
		else { printf("Other choice!\n"); break;}
	}
	close(sock);
	printf("Client [%d] disconnected\n", sock);
}

int login(int sock){
	int type, acc_no, fd, valid=1, invalid=0, login_success=0;
	char password[PASS_LENGTH];
	struct account temp;
	read(sock, &type, sizeof(type));
	if(type == 4) {return 0;}
	read(sock, &acc_no, sizeof(acc_no));
	read(sock, &password, sizeof(password));

	if((fd = open(ACCOUNT[type-1], O_RDWR))==-1)printf("File Error\n");
	lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_CUR);
	struct flock lock;
	
	lock.l_start = (acc_no-1)*sizeof(struct account);
	lock.l_len = sizeof(struct account);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	if(type == 1){
		// lock.l_type = F_RDLCK;
		lock.l_type = F_WRLCK;
		
		
		fcntl(fd,F_SETLKW, &lock);
		read(fd, &temp, sizeof(struct account));
		// lock.l_type = F_UNLCK;
		// fcntl(fd, F_SETLK, &lock);
		// close(fd);
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock, &valid, sizeof(valid));
				while(-1!=menu1(sock, temp.id, type));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	}
	else if(type == 2){
		lock.l_type = F_RDLCK;
		fcntl(fd,F_SETLKW,&lock);
		// lseek(fd,(acc_no-1)*sizeof(account),SEEK_CUR)
		read(fd, &temp, sizeof(struct account));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);

		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock, &valid, sizeof(valid));
				while(-1!=menu1(sock, temp.id, type));
				return 3;
			}
		}	
		if(login_success)
		return 3;
	}
	else if(type == 3){
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLKW, &lock);
		// lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(struct account));
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock, &valid, sizeof(valid));
				while(-1!=display_menu_admin(sock, temp.id));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	}
	write(sock, &invalid, sizeof(invalid));
	return 3;
}

int signup(int sock){
	int type, fd, acc_no=0;
	char password[PASS_LENGTH], name[10];
	struct account temp;

	read(sock, &type, sizeof(type));	//1:customer 2:Agent 3:Admin 4:Back
	if(type == 4){ return 0;}	//when back is chose
	read(sock, &name, sizeof(name));
	read(sock, &password, sizeof(password));

	if((fd = open(ACCOUNT[type-1], O_RDWR))==-1){
		printf("File Error\n");
		ERR_EXIT("open()");
	}
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	fcntl(fd,F_SETLKW, &lock);

	int fp = lseek(fd, 0, SEEK_END);

	if(fp==0){
		temp.id = 1;
	}
	else{
		fp = lseek(fd, -1 * sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(temp));
		temp.id++;
	}
	strcpy(temp.name, name);
	strcpy(temp.pass, password);
	write(fd, &temp, sizeof(temp));
	write(sock, &temp.id, sizeof(temp.id));

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);

	close(fd);
	return 3;
}

int display_menu_admin(int sock, int id){
	int op_id;
	read(sock, &op_id, sizeof(op_id));
	if(op_id == 1)
	{
		//add a train
		int tid = 0;
		int tno; 
		char tname[20];
		read(sock, &tname, sizeof(tname));
		read(sock, &tno, sizeof(tno));
		struct train temp, temp2;

		temp.tid = tid;
		temp.train_no = tno;
		strcpy(temp.train_name, tname);
		temp.av_seats = 15;
		temp.last_seatno_used = 0;

		int fd = open(TRAIN, O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		if(fp == 0){
			write(fd, &temp, sizeof(temp));
			write(sock, &op_id, sizeof(op_id));
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
		}
		else{
			lseek(fd, -1 * sizeof(struct train), SEEK_CUR);
			read(fd, &temp2, sizeof(temp2));
			temp.tid = temp2.tid + 1;
			write(fd, &temp, sizeof(temp));
			write(sock, &op_id, sizeof(op_id));	
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
		}
		return op_id;
	}
	if(op_id == 2)
	{
		int fd = open(TRAIN, O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		int train_id;
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR))
		{
			struct train temp;
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.train_no, sizeof(int));			
		}
		read(sock, &train_id, sizeof(int));
		if(train_id <= 0 || train_id > no_of_trains) 
		{
			train_id = 0;	//to abort operation
			write(sock, &train_id, sizeof(int));
		}
		else
		{
			struct train temp;
			lseek(fd, 0, SEEK_SET);
			lseek(fd, (train_id-1)*sizeof(struct train), SEEK_CUR);
			read(fd, &temp, sizeof(struct train));			
			printf("%s is deleted\n", temp.train_name);
			strcpy(temp.train_name,"deleted");
			lseek(fd, -1*sizeof(struct train), SEEK_CUR);
			write(fd, &temp, sizeof(struct train));
			write(sock, &train_id, sizeof(int));
		}

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
	}
	if(op_id == 3)
	{
		int fd = open(TRAIN, O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		int train_id;
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR))
		{
			struct train temp;
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.train_no, sizeof(int));			
		}
		read(sock, &train_id, sizeof(int));	//0: cancel, others : train_ID

		if(train_id <= 0 || train_id > no_of_trains)
		{
			train_id = 0;
			write(sock,&train_id,sizeof(train_id)); //not valid and abort operation
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
			return op_id;
		}
		else
		{
			write(sock,&train_id,sizeof(train_id)); //valid operation
			int choice;
			struct train temp;
			lseek(fd, 0, SEEK_SET);
			lseek(fd, (train_id-1)*sizeof(struct train), SEEK_CUR);
			read(fd, &temp, sizeof(struct train));			

			read(sock, &choice,sizeof(int));	//1: name,2: train no, 3: seats
			if(choice == 1)
			{
				char name[20];
				write(sock, &temp.train_name, sizeof(temp.train_name));
				read(sock, &name, sizeof(name));
				strcpy(temp.train_name, name);
			}
			else if(choice == 2){
			
				write(sock, &temp.av_seats, sizeof(temp.av_seats));
				read(sock, &temp.av_seats, sizeof(temp.av_seats));
			}

			printf("%s\t%d\t%d\n", temp.train_name, temp.train_no, temp.av_seats);
			lseek(fd, -1*sizeof(struct train), SEEK_CUR);
			write(fd, &temp, sizeof(struct train));
			write(sock, &choice, sizeof(int));

			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
			return op_id;
		}
	}
	if(op_id == 4)
	{
		
		return 0;
	}
	if(op_id == 5)
	{
		int type, id;
		struct account var;
		read(sock, &type, sizeof(type));

		int fd = open(ACCOUNT[type - 1], O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0 , SEEK_END);
		int users = fp/ sizeof(struct account);
		write(sock, &users, sizeof(int));

		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR))
		{
			read(fd, &var, sizeof(struct account));
			write(sock, &var.id, sizeof(var.id));
			write(sock, &var.name, sizeof(var.name));
		}

		read(sock, &id, sizeof(id));
		if(id == 0 || id > users)
		{
			id =0;
			write(sock, &id, sizeof(op_id));
		}
		else
		{
			lseek(fd, 0, SEEK_SET);
			lseek(fd, (id-1)*sizeof(struct account), SEEK_CUR);
			read(fd, &var, sizeof(struct account));
			lseek(fd, -1*sizeof(struct account), SEEK_CUR);
			strcpy(var.name,"deleted");
			strcpy(var.pass, "");
			write(fd, &var, sizeof(struct account));
			write(sock, &op_id, sizeof(op_id));
		}

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		close(fd);

		return op_id;
	}

	if(op_id == 6) 
	{
		write(sock,&op_id, sizeof(op_id));
		return -1;
	}
	
	
}

int menu1(int sock, int id, int type){
	int op_id;
	read(sock, &op_id, sizeof(op_id));
	if(op_id == 1){
		//book a ticket
		int fd = open(TRAIN, O_RDWR);
		int t_cnt;
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		struct train temp;
		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		
		struct train av_trains[no_of_trains];
		write(sock, &no_of_trains, sizeof(int));
		printf("No of trains : %d\n",no_of_trains);
		lseek(fd, 0, SEEK_SET);
		t_cnt=0;
		while(fp != lseek(fd, 0, SEEK_CUR)){
			read(fd, &temp, sizeof(struct train));
			av_trains[t_cnt++] = temp;
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_no, sizeof(int));	
			write(sock, &temp.av_seats, sizeof(int));	
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.last_seatno_used, sizeof(temp.last_seatno_used));
		}

		int trainid, seats;
		read(sock, &trainid, sizeof(trainid));

		int valid=0;
		char *del = "deleted";
		for(int i =0; i<t_cnt;i++){
			if(av_trains[i].tid+1 == trainid){
				printf("%d %s \n",av_trains[i].tid,av_trains[i].train_name);
				if(strcmp(av_trains[i].train_name,del) == 0)
					valid=0;
				else
					valid=1;
				break;
			}
		}

		write(sock,&valid,sizeof(valid));
		if(valid)
		{

			lseek(fd, 0, SEEK_SET);
			lseek(fd, (trainid - 1)*sizeof(struct train), SEEK_CUR);
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.av_seats, sizeof(int));
			printf("Train ID: %d, fetchedd ID: %d\n",trainid,temp.tid);
			read(sock, &seats, sizeof(seats));
			if(seats>0){
				temp.av_seats -= seats;
				int fd2 = open(BOOKING, O_RDWR);
				fcntl(fd2, F_SETLKW, &lock);
				struct bookings bk;
				int fp2 = lseek(fd2, 0, SEEK_END);
				if(fp2 > 0)
				{
					lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
					read(fd2, &bk, sizeof(struct bookings));
					bk.bid++;
				}
				else 
					bk.bid = 0;
				bk.type = type;
				bk.acc_no = id;
				bk.tr_id = trainid;
				bk.cancelled = 0;
				strcpy(bk.trainname, temp.train_name);
				bk.seat_start = temp.last_seatno_used + 1;
				bk.seat_end = temp.last_seatno_used + seats;
				temp.last_seatno_used = bk.seat_end;
				write(fd2, &bk, sizeof(bk));
				lock.l_type = F_UNLCK;
				fcntl(fd2, F_SETLK, &lock);
				close(fd2);
			}
			lseek(fd, -1*sizeof(struct train), SEEK_CUR);
			write(fd, &temp, sizeof(temp));
			if(seats<=0)
				op_id = -1;
			write(sock, &op_id, sizeof(op_id));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		return 1;
	}

	if(op_id == 2)
	{
		view_booking(sock, id, type);
		write(sock, &op_id, sizeof(op_id));
		return 2;
	}
	if(op_id == 3)
	{
		//update booking
		view_booking(sock, id, type);

		int fd1 = open(TRAIN, O_RDWR);
		int fd2 = open(BOOKING, O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd1, F_SETLKW, &lock);
		fcntl(fd2, F_SETLKW, &lock);

		int val;
		struct train temp1;
		struct bookings temp2;
		read(sock, &val, sizeof(int));	//Read the Booking ID to updated
		// read the booking to be updated
		lseek(fd2, 0, SEEK_SET);
		lseek(fd2, val*sizeof(struct bookings), SEEK_CUR);
		read(fd2, &temp2, sizeof(temp2));
		lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
		printf("%d %s %d\n", temp2.tr_id, temp2.trainname, temp2.seat_end);
		// read the train details of the booking
		lseek(fd1, 0, SEEK_SET);
		lseek(fd1, (temp2.tr_id-1)*sizeof(struct train), SEEK_CUR);
		read(fd1, &temp1, sizeof(temp1));
		lseek(fd1, -1*sizeof(struct train), SEEK_CUR);
		printf("%d %s %d\n", temp1.tid, temp1.train_name, temp1.av_seats);


		read(sock, &val, sizeof(int));	//Increase or Decrease


		if(val==1)
		{//increase
			read(sock, &val, sizeof(int)); //No of Seats
			if(temp1.av_seats>= val){
				temp2.cancelled = 1;
				temp1.av_seats += val;
				write(fd2, &temp2, sizeof(temp2));

				int tot_seats = temp2.seat_end - temp2.seat_start + 1 + val;
				struct bookings bk;

				int fp2 = lseek(fd2, 0, SEEK_END);
				lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
				read(fd2, &bk, sizeof(struct bookings));

				bk.bid++;
				bk.type = temp2.type;
				bk.acc_no = temp2.acc_no;
				bk.tr_id = temp2.tr_id;
				bk.cancelled = 0;
				strcpy(bk.trainname, temp2.trainname);
				bk.seat_start = temp1.last_seatno_used + 1;
				bk.seat_end = temp1.last_seatno_used + tot_seats;

				temp1.av_seats -= tot_seats;
				temp1.last_seatno_used = bk.seat_end;

				write(fd2, &bk, sizeof(bk));
				write(fd1, &temp1, sizeof(temp1));
			}
			else{
				op_id = -2;
				write(sock, &op_id, sizeof(op_id));
			}
		}
		else{//decrease			
			read(sock, &val, sizeof(int)); //No of Seats
			if(temp2.seat_end - val < temp2.seat_start){
				temp2.cancelled = 1;
				temp1.av_seats += val;
			}
			else{
				temp2.seat_end -= val;
				temp1.av_seats += val;
			}
			write(fd2, &temp2, sizeof(temp2));
			write(fd1, &temp1, sizeof(temp1));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd1, F_SETLK, &lock);
		fcntl(fd2, F_SETLK, &lock);
		close(fd1);
		close(fd2);
		if(op_id>0)
		write(sock, &op_id, sizeof(op_id));
		return 3;

	}
	if(op_id == 4) {
		//cancel booking
		view_booking(sock,id,type);
		int bid;
		struct bookings booking;
		struct train tr;
		read(sock,&bid,sizeof(bid));

		int fd = open(BOOKING, O_RDWR);
		struct flock lock,lock2;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd, F_SETLKW, &lock);

		lseek(fd,(bid-1)*sizeof(struct bookings),SEEK_SET);

		read(fd,&booking,sizeof(struct bookings));
		
		if(booking.cancelled == 0 && booking.acc_no == id && booking.type == type){


			booking.cancelled = 1;
			int fd2 = open(TRAIN,O_RDWR);
			
			fcntl(fd2,F_SETLKW,&lock);
			lseek(fd2,(booking.tr_id -1)*sizeof(struct train),SEEK_SET);
			read(fd2,&tr,sizeof(struct train));
			tr.av_seats += (booking.seat_end-booking.seat_start+1);
			lseek(fd2,-1*sizeof(struct train),SEEK_CUR);
			write(fd2,&tr,sizeof(struct train));

			lock.l_type = F_UNLCK;
			fcntl(fd2,F_SETLK,&lock);
			close(fd2);
			write(sock, &op_id, sizeof(op_id));
		
		}
		else{
			op_id = 0;
			write(sock, &op_id, sizeof(op_id));
		}
		lseek(fd,-1 * sizeof(struct bookings),SEEK_CUR);
		write(fd,&booking,sizeof(struct bookings));
		lock.l_type = F_UNLCK;
		fcntl(fd,F_SETLK,&lock);
		close(fd);

		return 4;
	}
	if(op_id == 5) {
		write(sock, &op_id, sizeof(op_id));
		return -1;
	}
	return 0;
}

void view_booking(int sock, int id, int type){
	int fd = open(BOOKING, O_RDONLY);
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	
	fcntl(fd, F_SETLKW, &lock);

	int fp = lseek(fd, 0, SEEK_END);
	int entries = 0;
	if(fp == 0)
		write(sock, &entries, sizeof(entries));
	else{
		struct bookings bk[10];
		while(fp>0 && entries<10){
			struct bookings temp;
			fp = lseek(fd, -1*sizeof(struct bookings), SEEK_CUR);
			read(fd, &temp, sizeof(struct bookings));
			if(temp.acc_no == id && temp.type == type)
				bk[entries++] = temp;
			fp = lseek(fd, -1*sizeof(struct bookings), SEEK_CUR);
		}
		write(sock, &entries, sizeof(entries));
		for(fp=0;fp<entries;fp++){
			write(sock, &bk[fp].bid, sizeof(bk[fp].bid));
			write(sock, &bk[fp].tr_id, sizeof(bk[fp].tr_id));
			write(sock, &bk[fp].seat_start, sizeof(int));
			write(sock, &bk[fp].seat_end, sizeof(int));
			write(sock, &bk[fp].cancelled, sizeof(int));
		}
	}
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

int main(){
	printf("Initializing connection...\n");
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		//if socket creation fails
		printf("socket creation failed\n");
		ERR_EXIT("socket()");
	}
	int optval = 1;
	int optlen = sizeof(optval);
	/*
	to close socket automatically while terminating process
	SOL_SOCKET : to manipulate option at API level o/w specify level
	*/
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, optlen)==-1){
		printf("set socket options failed\n");
		ERR_EXIT("setsockopt()");
	}
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(PORT);
	/*
	sockaddr_in is from family AF_INET (ip(7)) and varies as per the adress family
	*/
	printf("Binding socket...\n");
	if(bind(sockfd, (struct sockaddr *)&sa, sizeof(sa))==-1){
		printf("binding port failed\n");
		ERR_EXIT("bind()");
	}
	//2nd arg : size of backlog
	if(listen(sockfd, 100)==-1){
		printf("listen failed\n");
		ERR_EXIT("listen()");
	}
	printf("Listening...\n");
	while(1){ 
		int connectedfd;
		if((connectedfd = accept(sockfd, (struct sockaddr *)NULL, NULL))==-1){
			printf("connection error\n");
			ERR_EXIT("accept()");
		}
		// pthread_t cli;
		if(fork()==0){
			service_cli(connectedfd);
			exit(1);
		}
	}
	close(sockfd);
	printf("Connection closed!\n");
	return 0;
}
