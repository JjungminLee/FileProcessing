#include <stdio.h> // 필요한 header file 추가 가능
#include "student.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct _Node
{
	STUDENT student;
	struct _Node *next;
} Node;

// 검색시 여러 레코드가 검색되면 링크드 리스트에 할당
void insertNode(Node **head, STUDENT student)
{
	Node *newNode = (Node *)malloc(sizeof(Node));
	if (newNode == NULL)
	{
		fprintf(stderr, "Memory allocation error\n");
		exit(1);
	}
	newNode->student = student;
	newNode->next = *head;
	*head = newNode;
}

int countNodes(Node *head)
{
	int count = 0;
	Node *current = head;
	while (current != NULL)
	{
		count++;
		current = current->next;
	}
	return count;
}

// readPage() 함수는 레코드 파일에서 주어진 페이지번호 pagenum(=0, 1, 2, ...)에 해당하는 page를
// 읽어서 pagebuf가 가리키는 곳에 저장하는 역할을 수행한다. 정상적인 수행을 한 경우
// '1'을, 그렇지 않은 경우는 '0'을 리턴한다.
// getRecFromPagebuf() 함수는 readPage()를 통해 읽어온 page에서 주어진 레코드번호 recordnum(=0, 1, 2, ...)에
// 해당하는 레코드를 recordbuf에 전달하는 일을 수행한다.
// 만약 page에서 전달할 record가 존재하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
// unpack() 함수는 recordbuf에 저장되어 있는 record에서 각 field를 추출하여 학생 객체에 저장하는 일을 한다.

int readPage(FILE *fp, char *pagebuf, int pagenum);
int getRecFromPagebuf(const char *pagebuf, char *recordbuf, int recordnum);
void unpack(const char *recordbuf, STUDENT *s);

//
// 주어진 학생 객체를 레코드 파일에 저장할 때 사용되는 함수들이며, 그 시나리오는 다음과 같다.
// 1. 학생 객체를 실제 레코드 형태의 recordbuf에 저장한다(pack() 함수 이용).
// 2. 레코드 파일의 맨마지막 page를 pagebuf에 읽어온다(readPage() 이용). 만약 새로운 레코드를 저장할 공간이
//    부족하면 pagebuf에 empty page를 하나 생성한다.
// 3. recordbuf에 저장되어 있는 레코드를 pagebuf의 page에 저장한다(writeRecToPagebuf() 함수 이용).
// 4. 수정된 page를 레코드 파일에 저장한다(writePage() 함수 이용)
//
// writePage()는 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//

int writePage(FILE *fp, const char *pagebuf, int pagenum);
void writeRecToPagebuf(FILE *fp, int pagenum, char *pagebuf, const char *recordbuf);
void pack(char *recordbuf, const STUDENT *s);

//
// 레코드 파일에서 field의 키값(keyval)을 갖는 레코드를 검색하고 그 결과를 출력한다.
// 검색된 레코드를 출력할 때 반드시 printSearchResult() 함수를 사용한다.
//
void search(FILE *fp, FIELD f, char *keyval);
void printSearchResult(const STUDENT *s, int n);

//
// 레코드 파일에 새로운 학생 레코드를 추가할 때 사용한다. 표준 입력으로 받은 필드값들을 이용하여
// 학생 객체로 바꾼 후 insert() 함수를 호출한다.
//

void insert(FILE *fp, const STUDENT *s);

//
// 레코드의 필드명을 enum FIELD 타입의 값으로 변환시켜 준다.
// 예를 들면, 사용자가 수행한 명령어의 인자로 "NAME"이라는 필드명을 사용하였다면
// 프로그램 내부에서 이를 NAME(=1)으로 변환할 필요성이 있으며, 이때 이 함수를 이용한다.
//

FIELD getFieldID(char *fieldname);

//
// 레코드 파일의 헤더를 읽어 오거나 수정할 때 사용한다.
// 예를 들어, 새로운 #pages를 수정하기 위해서는 먼저 readFileHeader()를 호출하여 헤더의 내용을
// headerbuf에 저장한 후 여기에 #pages를 수정하고 writeFileHeader()를 호출하여 변경된 헤더를
// 저장한다. 두 함수 모두 성공하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//

int readFileHeader(FILE *fp, char *headerbuf);
int writeFileHeader(FILE *fp, const char *headerbuf);

//

int readFileHeader(FILE *fp, char *headerbuf)
{

	int ret;

	fseek(fp, 0, SEEK_SET);
	ret = fread((void *)headerbuf, FILE_HEADER_SIZE, 1, fp);
	if (ret == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int writeFileHeader(FILE *fp, const char *headerbuf)
{
	int ret;

	fseek(fp, 0, SEEK_SET);
	ret = fwrite((void *)headerbuf, FILE_HEADER_SIZE, 1, fp);
	if (ret == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int readPage(FILE *fp, char *pagebuf, int pagenum)
{
	int ret;

	fseek(fp, PAGE_SIZE * pagenum + FILE_HEADER_SIZE, SEEK_SET);
	ret = fread((void *)pagebuf, PAGE_SIZE, 1, fp);
	if (ret == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
int getRecFromPagebuf(const char *pagebuf, char *recordbuf, int recordnum)
{
	unsigned short offset;
	unsigned short next_offset;

	// 첫 번째 레코드의 경우, 시작 오프셋을 PAGE_HEADER_SIZE로 설정
	if (recordnum == 0)
	{
		offset = PAGE_HEADER_SIZE;
	}
	else
	{
		// 현재 레코드의 시작위치 알기 위해 이전 레코드의 끝 오프셋을 가져옴
		memcpy(&offset, pagebuf + 8 + (recordnum - 1) * sizeof(unsigned short), sizeof(unsigned short));
		// 시작 오프셋을 계산하기 위해 1을 더함
		offset++;
	}

	// 현재 레코드의 끝 오프셋을 가져옴
	memcpy(&next_offset, pagebuf + 8 + recordnum * sizeof(unsigned short), sizeof(unsigned short));
	// 레코드의 길이를 계산
	int length = next_offset - offset + 1;

	// 레코드를 페이지 버퍼에서 복사하여 recordbuf에 저장
	memcpy(recordbuf, pagebuf + offset, length);
	// recordbuf를 문자열로 만듦
	recordbuf[length] = '\0';

	return 1;
}
void unpack(const char *recordbuf, STUDENT *s)
{
	char *tempbuf = strdup(recordbuf); // recordbuf를 수정하지 않도록 복사
	char *token = strtok(tempbuf, "#");

	if (token != NULL)
		strncpy(s->id, token, sizeof(s->id) - 1);
	s->id[sizeof(s->id) - 1] = '\0';

	token = strtok(NULL, "#");
	if (token != NULL)
		strncpy(s->name, token, sizeof(s->name) - 1);
	s->name[sizeof(s->name) - 1] = '\0';

	token = strtok(NULL, "#");
	if (token != NULL)
		strncpy(s->dept, token, sizeof(s->dept) - 1);
	s->dept[sizeof(s->dept) - 1] = '\0';

	token = strtok(NULL, "#");
	if (token != NULL)
		strncpy(s->year, token, sizeof(s->year) - 1);
	s->year[sizeof(s->year) - 1] = '\0';

	token = strtok(NULL, "#");
	if (token != NULL)
		strncpy(s->addr, token, sizeof(s->addr) - 1);
	s->addr[sizeof(s->addr) - 1] = '\0';

	token = strtok(NULL, "#");
	if (token != NULL)
		strncpy(s->phone, token, sizeof(s->phone) - 1);
	s->phone[sizeof(s->phone) - 1] = '\0';

	token = strtok(NULL, "#");
	if (token != NULL)
		strncpy(s->email, token, sizeof(s->email) - 1);
	s->email[sizeof(s->email) - 1] = '\0';

	free(tempbuf);
}
void pack(char *recordbuf, const STUDENT *s)
{
	sprintf(recordbuf, "%s#%s#%s#%s#%s#%s#%s#",
			s->id, s->name, s->dept, s->year, s->addr, s->phone, s->email);
}
int writePage(FILE *fp, const char *pagebuf, int pagenum)
{

	int ret;

	fseek(fp, PAGE_SIZE * pagenum + FILE_HEADER_SIZE, SEEK_SET);
	ret = fwrite((void *)pagebuf, PAGE_SIZE, 1, fp);

	if (ret == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
// 레코드를 페이지 버퍼에 기록하는 함수 -> writePage까지 함께
void insert(FILE *fp, const STUDENT *s)
{
	char fileHeaderBuf[FILE_HEADER_SIZE];
	char recordbuf[MAX_RECORD_SIZE];
	char pagebuf[PAGE_SIZE];

	// 파일 크기를 확인하여 파일이 비어 있는지 확인
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned short header_total_pages;

	if (fileSize == 0)
	{
		// 파일이 비어 있으면 파일 헤더와 첫 페이지를 초기화
		memset(fileHeaderBuf, 0, FILE_HEADER_SIZE);
		header_total_pages = 0;
		memcpy(fileHeaderBuf, &header_total_pages, sizeof(unsigned short));
		if (writeFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write file header\n");
			fclose(fp);
			exit(1);
		}
		// 여기까지 헤더

		memset(pagebuf, 0, PAGE_SIZE);
		unsigned short total_records = 0;
		unsigned short initial_freespace = PAGE_SIZE - PAGE_HEADER_SIZE;
		int reserved_space = 0;
		memcpy(pagebuf, &total_records, sizeof(unsigned short));
		memcpy(pagebuf + sizeof(unsigned short), &initial_freespace, sizeof(unsigned short));
		memcpy(pagebuf + 2 * sizeof(unsigned short), &reserved_space, sizeof(int));
		if (writePage(fp, pagebuf, 0) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write initial page\n");
			fclose(fp);
			exit(1);
		}

		memset(recordbuf, 0, MAX_RECORD_SIZE);
		pack(recordbuf, s);

		memset(fileHeaderBuf, 0, FILE_HEADER_SIZE);
		if (readFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to read file header\n");
			fclose(fp);
			exit(1);
		}
		unsigned short header_total_pages1;
		memcpy(&header_total_pages1, fileHeaderBuf, sizeof(unsigned short));

		writeRecToPagebuf(fp, header_total_pages, pagebuf, recordbuf);
	}
	else
	{
		// 파일 헤더 읽기
		memset(fileHeaderBuf, 0, FILE_HEADER_SIZE);
		if (readFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to read file header\n");
			fclose(fp);
			exit(1);
		}

		memcpy(&header_total_pages, fileHeaderBuf, sizeof(unsigned short));

		memset(pagebuf, 0, PAGE_SIZE);
		// 마지막 페이지 읽기
		if (readPage(fp, pagebuf, header_total_pages) != 1)
		{
			fprintf(stderr, "ERROR: read page error\n");
			exit(1);
		}

		memset(recordbuf, 0, MAX_RECORD_SIZE);
		pack(recordbuf, s);

		writeRecToPagebuf(fp, header_total_pages, pagebuf, recordbuf);
	}
}

// 레코드를 페이지 버퍼에 기록하는 함수 -> writePage까지 함께
void writeRecToPagebuf(FILE *fp, int pagenum, char *pagebuf, const char *recordbuf)
{
	unsigned short total_records; // 페이지에 저장된 총 레코드 수
	unsigned short freespace;	  // 페이지의 여유 공간
	int reserved_space;			  // 예약된 공간

	// 페이지의 총 레코드 수와 여유 공간 읽기
	memcpy(&total_records, pagebuf, sizeof(unsigned short));
	memcpy(&freespace, pagebuf + sizeof(unsigned short), sizeof(unsigned short));
	memcpy(&reserved_space, pagebuf + 2 * sizeof(unsigned short), sizeof(int));

	unsigned short record_length = strlen(recordbuf); // 레코드의 길이

	// 3-1) 레코드가 현재 페이지의 여유 공간보다 큰 경우, 새로운 페이지에 저장
	if (record_length + sizeof(unsigned short) > freespace)
	{

		// 파일 헤더의 총 페이지 수를 업데이트
		pagenum++;

		// 새로운 페이지 초기화
		memset(pagebuf, 0, PAGE_SIZE);
		total_records = 0;
		freespace = PAGE_SIZE - PAGE_HEADER_SIZE;
		reserved_space = 0;

		memcpy(pagebuf, &total_records, sizeof(unsigned short));
		memcpy(pagebuf + sizeof(unsigned short), &freespace, sizeof(unsigned short));
		memcpy(pagebuf + 2 * sizeof(unsigned short), &reserved_space, sizeof(int));

		// 파일 헤더 업데이트
		char fileHeaderBuf[FILE_HEADER_SIZE];
		unsigned short header_total_pages;

		if (readFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to read file header\n");
			exit(1);
		}

		memcpy(&header_total_pages, fileHeaderBuf, sizeof(unsigned short));
		header_total_pages++;
		memcpy(fileHeaderBuf, &header_total_pages, sizeof(unsigned short));

		if (writeFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write file header\n");
			exit(1);
		}

		// 현재 페이지를 파일에 기록
		if (writePage(fp, pagebuf, pagenum) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write page\n");
			exit(1);
		}
	}

	// 3-2) free space에 레코드가 들어갈 수 있는 경우 pagebuf에 저장
	unsigned short offset;
	if (total_records == 0)
	{
		// 첫 번째 레코드인 경우
		offset = PAGE_HEADER_SIZE;
	}
	else
	{
		// 이전 레코드의 끝 위치를 가져와서 새로운 레코드의 시작 위치를 계산
		unsigned short last_record_end = PAGE_HEADER_SIZE - 1;

		memcpy(&last_record_end, pagebuf + 8 + (total_records - 1) * sizeof(unsigned short), sizeof(unsigned short));

		offset = last_record_end + 1;
	}

	// 레코드 저장
	strncpy(pagebuf + offset, recordbuf, record_length);

	// 오프셋 업데이트 (마지막 바이트 주소를 기록)
	unsigned short record_end = offset + record_length - 1;
	memcpy(pagebuf + 8 + total_records * sizeof(unsigned short), &record_end, sizeof(unsigned short));

	// 헤더 업데이트
	total_records++;			// 총 레코드 수 증가
	freespace -= record_length; // 여유 공간 감소

	memcpy(pagebuf, &total_records, sizeof(unsigned short));
	memcpy(pagebuf + sizeof(unsigned short), &freespace, sizeof(unsigned short));
	memcpy(pagebuf + 2 * sizeof(unsigned short), &reserved_space, sizeof(int));

	// 현재 페이지를 파일에 기록
	if (writePage(fp, pagebuf, pagenum) != 1)
	{
		fprintf(stderr, "ERROR: Failed to write page\n");
		exit(1);
	}
}

void search(FILE *fp, FIELD f, char *keyval)
{
	Node *head = NULL;
	// 1) 파일 헤더 가져와서 페이지 수 가져오기
	char fileHeaderBuf[FILE_HEADER_SIZE];
	char recordbuf[MAX_RECORD_SIZE];
	char pagebuf[PAGE_SIZE];
	STUDENT student;
	// 레코드 파일 헤더 가져오기
	if (readFileHeader(fp, fileHeaderBuf) != 1)
	{
		fprintf(stderr, "ERROR: Failed to write file header\n");
		fclose(fp);
		exit(1);
	}
	unsigned short header_total_pages;
	memcpy(&header_total_pages, fileHeaderBuf, sizeof(unsigned short));

	// 2) 페이지 하나씩 방문해서 데이터영역의 레코드에서 필드와 keyval에 해당하는거 찾기
	for (int i = 0; i <= header_total_pages; i++)
	{
		memset(pagebuf, 0, PAGE_SIZE);
		// i번째 페이지 가져오기
		if (readPage(fp, pagebuf, i) != 1)
		{
			fprintf(stderr, "ERROR: read page error\n");
			exit(1);
		}
		// 페이지에서 레코드 한개씩 방문해서 keyval에 해당하는거 찾기

		unsigned short numRecords;
		memcpy(&numRecords, pagebuf, sizeof(unsigned short)); // 페이지 헤더 첫 번째 2바이트에서 총 레코드 수 읽기

		for (int j = 0; j < numRecords; j++)
		{
			getRecFromPagebuf(pagebuf, recordbuf, j);

			unpack(recordbuf, &student);

			int match = 0;
			switch (f)
			{
			case ID:
				if (strcmp(student.id, keyval) == 0)
					match = 1;
				break;
			case NAME:
				if (strcmp(student.name, keyval) == 0)
					match = 1;
				break;
			case DEPT:
				if (strcmp(student.dept, keyval) == 0)
					match = 1;
				break;
			case YEAR:
				if (strcmp(student.year, keyval) == 0)
					match = 1;
				break;
			case ADDR:
				if (strcmp(student.addr, keyval) == 0)
					match = 1;
				break;
			case PHONE:
				if (strcmp(student.phone, keyval) == 0)
					match = 1;
				break;
			case EMAIL:
				if (strcmp(student.email, keyval) == 0)
					match = 1;
				break;
			default:
				fprintf(stderr, "Unknown field\n");
				break;
			}
			// 3) 링크드 리스트에 넣어두기
			if (match)
			{

				insertNode(&head, student);
			}
		}
	}

	// 4) 검색 결과 출력
	int totalResults = countNodes(head);
	Node *current = head;
	STUDENT *results = malloc(totalResults * sizeof(STUDENT));
	int index = 0;
	while (current != NULL)
	{
		results[index++] = current->student;
		current = current->next;
	}
	printSearchResult(results, totalResults);
	free(results);

	// 메모리 해제
	current = head;
	while (current != NULL)
	{
		Node *next = current->next;
		free(current);
		current = next;
	}
}

FIELD getFieldID(char *fieldname)
{
	if (!strcmp(fieldname, "ID"))
		return ID;
	if (!strcmp(fieldname, "NAME"))
		return NAME;
	if (!strcmp(fieldname, "DEPT"))
		return DEPT;
	if (!strcmp(fieldname, "YEAR"))
		return YEAR;
	if (!strcmp(fieldname, "ADDR"))
		return ADDR;
	if (!strcmp(fieldname, "PHONE"))
		return PHONE;
	if (!strcmp(fieldname, "EMAIL"))
		return EMAIL;
	return 0;
}
void printSearchResult(const STUDENT *s, int n)
{
	int i;

	printf("#Records = %d\n", n);

	for (i = 0; i < n; i++)
	{
		printf("%s#%s#%s#%s#%s#%s#%s\n", s[i].id, s[i].name, s[i].dept, s[i].year, s[i].addr, s[i].phone, s[i].email);
	}
}

int main(int argc, char *argv[])
{
	FILE *fp; // 모든 file processing operation은 C library를 사용할 것
	char record_file[FILE_HEADER_SIZE];

	int opt;
	char *fileName;
	char fileHeaderBuf[FILE_HEADER_SIZE];
	char pageHeaderBuf[PAGE_HEADER_SIZE];
	char pagebuf[PAGE_SIZE - PAGE_HEADER_SIZE];
	STUDENT student;
	if (argc < 3)
	{
		fprintf(stderr, "Usage : %s -a record_file_name \"val1\" \"val2\" \"val3\" \"val4\" \"val5\" \n", argv[0]);
		exit(1);
	}

	while ((opt = getopt(argc, argv, "i:s:")) != -1)
	{

		fileName = argv[2];

		if (opt == 'i')
		{

			fp = fopen(fileName, "rb+");
			if (fp == NULL)
			{
				fp = fopen(fileName, "wb+");
				if (fp == NULL)
				{
					fprintf(stderr, "ERROR: fopen error\n");
					exit(1);
				}
				fclose(fp);
				fp = fopen(fileName, "rb+");
				if (fp == NULL)
				{
					fprintf(stderr, "ERROR: fopen error\n");
					exit(1);
				}
			}

			// 1)student구조체에 pack하기
			memset(&student, 0, sizeof(STUDENT));

			for (int i = 3; i < argc; i++)
			{
				char *field = strtok(argv[i], "=");
				char *value = strtok(NULL, "=");
				FIELD fieldID = getFieldID(field);
				switch (fieldID)
				{
				case ID:
					strcpy(student.id, value);
					break;
				case NAME:
					strcpy(student.name, value);
					break;
				case DEPT:
					strcpy(student.dept, value);
					break;
				case YEAR:
					strcpy(student.year, value);
					break;
				case ADDR:
					strcpy(student.addr, value);
					break;
				case PHONE:
					strcpy(student.phone, value);
					break;
				case EMAIL:
					strcpy(student.email, value);
					break;
				default:
					fprintf(stderr, "ERROR: Unknown field: %s\n", field);
					fclose(fp);
					exit(1);
				}
			}
			// 2) pack한 데이터를 insert함수 내부에서 record buf만들어서 writeRecPageBuf호출
			insert(fp, &student);

			fclose(fp);
		}
		else if (opt == 's')
		{

			if ((fp = fopen(fileName, "rb")) < 0)
			{
				fprintf(stderr, "ERROR: fopen error\n");
				exit(1);
			}
			char *field = strtok(argv[3], "=");
			char *value = strtok(NULL, "=");
			FIELD fieldID = getFieldID(field);
			switch (fieldID)
			{
			case ID:
				strcpy(student.id, value);
				break;
			case NAME:
				strcpy(student.name, value);
				break;
			case DEPT:
				strcpy(student.dept, value);
				break;
			case YEAR:
				strcpy(student.year, value);
				break;
			case ADDR:
				strcpy(student.addr, value);
				break;
			case PHONE:
				strcpy(student.phone, value);
				break;
			case EMAIL:
				strcpy(student.email, value);
				break;
			default:
				fprintf(stderr, "ERROR: Unknown field: %s\n", field);
				fclose(fp);
				exit(1);
			}

			search(fp, fieldID, value);
		}
	}

	exit(0);
}
