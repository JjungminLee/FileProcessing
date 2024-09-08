#include <stdio.h>
#include "student.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct _Node
{
	STUDENT student;
	struct _Node *next;
} Node;
typedef struct _DeleteNode
{
	short page_num;			  // 삭제된 레코드의 페이지 번호
	short record_num;		  // 페이지 내에서의 삭제된 레코드 번호
	struct _DeleteNode *prev; // 리스트 내 이전 노드를 가리키는 포인터
	struct _DeleteNode *next; // 리스트 내 다음 노드를 가리키는 포인터
} DeleteNode;

void insertDeleteNode(DeleteNode **head, DeleteNode **tail, short page_num, short record_num)
{
	DeleteNode *newNode = (DeleteNode *)malloc(sizeof(DeleteNode));
	if (newNode == NULL)
	{
		fprintf(stderr, "Memory allocation error\n");
		exit(1);
	}
	newNode->page_num = page_num;
	newNode->record_num = record_num;
	newNode->next = NULL;
	newNode->prev = NULL;

	if (*head == NULL)
	{
		*head = newNode;
		*tail = newNode;
	}
	else
	{
		newNode->next = *head;
		(*head)->prev = newNode;
		*head = newNode;
	}
}

void deleteDeleteNode(DeleteNode **head, DeleteNode **tail, short page_num, short record_num)
{
	DeleteNode *current = *head;

	while (current != NULL)
	{
		if (current->page_num == page_num && current->record_num == record_num)
		{
			if (current->prev)
			{
				current->prev->next = current->next;
			}
			else
			{
				*head = current->next;
			}
			if (current->next)
			{
				current->next->prev = current->prev;
			}
			else
			{
				*tail = current->prev;
			}
			free(current);
			return;
		}
		current = current->next;
	}
}

DeleteNode *deleteHead = NULL;
DeleteNode *deleteTail = NULL;
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

int readPage(FILE *fp, char *pagebuf, int pagenum);
int getRecFromPagebuf(const char *pagebuf, char *recordbuf, int recordnum);
void unpack(const char *recordbuf, STUDENT *s);

int writePage(FILE *fp, const char *pagebuf, int pagenum);
void writeRecToPagebuf(char *pagebuf, const char *recordbuf);
void pack(char *recordbuf, const STUDENT *s);

void search(FILE *fp, FIELD f, char *keyval);
void printSearchResult(const STUDENT *s, int n);

void insert(FILE *fp, const STUDENT *s);

FIELD getFieldID(char *fieldname);

int readFileHeader(FILE *fp, char *headerbuf);
int writeFileHeader(FILE *fp, const char *headerbuf);

void delete(FILE *fp, char *keyval);

int searchByID(FILE *fp, char *keyval, char *recordbuf, int *pagenum, int *recordnum);

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
	short offset;
	short next_offset;

	// 첫 번째 레코드의 경우, 시작 오프셋을 PAGE_HEADER_SIZE로 설정
	if (recordnum == 0)
	{
		offset = PAGE_HEADER_SIZE;
	}
	else
	{
		// 현재 레코드의 시작위치 알기 위해 이전 레코드의 끝 오프셋을 가져옴
		memcpy(&offset, pagebuf + 8 + (recordnum - 1) * sizeof(short), sizeof(short));
		// 시작 오프셋을 계산하기 위해 1을 더함
		offset++;
	}

	// 현재 레코드의 끝 오프셋을 가져옴
	memcpy(&next_offset, pagebuf + 8 + recordnum * sizeof(short), sizeof(short));
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
	DeleteNode *current = NULL;

	// 파일 크기를 확인하여 파일이 비어 있는지 확인
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	short header_total_pages;
	short delete_list_front_pagenum;
	short delete_list_front_recordnum;

	if (fileSize == 0)
	{
		// 파일이 비어 있으면 파일 헤더와 첫 페이지를 초기화
		memset(fileHeaderBuf, 0, FILE_HEADER_SIZE);
		header_total_pages = 0;
		// 바로 직전에 삭제된 레코드가 존재하지 않으면 Page_num과 Record_num는 모두 -1의 값을 가진다.
		delete_list_front_pagenum = -1;
		delete_list_front_recordnum = -1;
		memcpy(fileHeaderBuf, &header_total_pages, sizeof(short));
		memcpy(fileHeaderBuf + sizeof(short), &delete_list_front_pagenum, sizeof(short));
		memcpy(fileHeaderBuf + 2 * sizeof(short), &delete_list_front_recordnum, sizeof(short));

		if (writeFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write file header\n");
			fclose(fp);
			exit(1);
		}
		// 여기까지 헤더

		memset(pagebuf, 0, PAGE_SIZE);
		short total_records = 0;
		short initial_freespace = PAGE_SIZE - PAGE_HEADER_SIZE;
		int reserved_space = 0;
		memcpy(pagebuf, &total_records, sizeof(short));
		memcpy(pagebuf + sizeof(short), &initial_freespace, sizeof(short));
		memcpy(pagebuf + 2 * sizeof(short), &reserved_space, sizeof(int));
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
		short header_total_pages1;
		memcpy(&header_total_pages1, fileHeaderBuf, sizeof(short));
		// 어차피 초기화가 목적이기 때문에 새 페이지 만들어지는 경우는 고려하지 않아도 됨
		writeRecToPagebuf(pagebuf, recordbuf);
		int pagenum = header_total_pages1;
		// 현재 페이지를 파일에 기록
		if (writePage(fp, pagebuf, pagenum) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write page\n");
			exit(1);
		}
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

		/*@todo*/
		// 1.삭제레코드 리스트를 먼저 확인
		// 2.삭제 레코드 리스트 헤드부터 순회하면서 pagenum이랑 recordnum으로 offset활용해서 length구한 후 insert하려는 레코드 사이즈가 삭제리스트 내 노드 중 하나와 작거나 같으면 그 노드에 써준다.
		// 삭제 리스트에서는 해당 노드를 삭제한다.
		// 만약 리스트의 가장 앞의 노드에 써진다면 파일 헤더도 변경해준다.
		// 3. 만족안하면 마지막 페이지에 넣는다. (이전 과제 로직 활용)
		memset(pagebuf, 0, PAGE_SIZE);
		// 삭제 레코드 리스트를 채우기
		// 파일헤더의 값을 확인후 헤더가 가리키는 페이지의 레코드로 감 -> 리스트에 insert
		// 그 레코드에서 *(2b) 페이지 번호 (2b) 레코드 번호 (2b) 추출 -> 해당 페이지번호에 해당하는 레코드 번호로 감 -> 리스트에 insert -1,-1나올때까지 반복
		memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));
		memcpy(&delete_list_front_pagenum, fileHeaderBuf + sizeof(short), sizeof(short));
		memcpy(&delete_list_front_recordnum, fileHeaderBuf + 2 * sizeof(short), sizeof(short));

		if (delete_list_front_pagenum == -1 && delete_list_front_recordnum == -1)
		{
		}
		else
		{
			// 삭제 레코드 확인해서 리스트에 넣어주기
			insertDeleteNode(&deleteHead, &deleteTail, delete_list_front_pagenum, delete_list_front_recordnum);
			short prevPageNum = delete_list_front_pagenum;
			short prevRecordNum = delete_list_front_recordnum;

			while (1) // 여기서 탈출 조건을 확인
			{

				memset(pagebuf, 0, PAGE_SIZE);
				memset(recordbuf, 0, MAX_RECORD_SIZE);

				if (readPage(fp, pagebuf, prevPageNum) != 1)
				{
					fprintf(stderr, "ERROR0: read page error\n");
					exit(1);
				}

				getRecFromPagebuf(pagebuf, recordbuf, prevRecordNum);
				// 그 레코드에서 *(2b) 페이지 번호 (2b) 레코드 번호 (2b) 추출

				memcpy(&prevPageNum, recordbuf + 1, sizeof(short));
				memcpy(&prevRecordNum, recordbuf + 1 + sizeof(short), sizeof(short));

				if (prevPageNum == -1 && prevRecordNum == -1)
				{
					break;
				}

				insertDeleteNode(&deleteHead, &deleteTail, prevPageNum, prevRecordNum);
			}
		}

		current = deleteTail;
		if (current != NULL)
		{
			while (current != NULL)
			{

				int pagenum = current->page_num;
				int recordnum = current->record_num;

				// 해당 pagenum에 해당하는 페이지를 pagebuf에 담아온다.
				if (readPage(fp, pagebuf, pagenum) == 0)
				{
					fprintf(stderr, "ERROR2: page read error\n");
					exit(1);
				}
				// pagebuf에 recordnum이 해당하는 offset위치에 record 덮어 쓴다.
				short offset;
				short next_offset;

				// 첫 번째 레코드의 경우, 시작 오프셋을 PAGE_HEADER_SIZE로 설정
				if (recordnum == 0)
				{
					offset = PAGE_HEADER_SIZE;
				}
				else
				{
					// 현재 레코드의 시작위치 알기 위해 이전 레코드의 끝 오프셋을 가져옴
					memcpy(&offset, pagebuf + 8 + (recordnum - 1) * sizeof(short), sizeof(short));
					// 시작 오프셋을 계산하기 위해 1을 더함
					offset++;
				}

				// 현재 레코드의 끝 오프셋을 가져옴
				memcpy(&next_offset, pagebuf + 8 + recordnum * sizeof(short), sizeof(short));
				// 레코드의 길이를 계산
				int deleteLength = next_offset - offset + 1;
				int actualLength = strlen(recordbuf);

				// insert하려는 레코드 사이즈가 삭제리스트 내 노드 중 하나와 작거나 같으면 그 노드에 써준다.
				if (deleteLength >= actualLength)
				{

					memset(recordbuf, 0, MAX_RECORD_SIZE);
					pack(recordbuf, s);
					strncpy(pagebuf + offset, recordbuf, actualLength);

					if (writePage(fp, pagebuf, pagenum) != 1)

					{
						fprintf(stderr, "Error: write page error\n");
						exit(1);
					}

					// 만약 리스트의 가장 앞의 노드에 써진다면 파일 헤더도 변경해준다. (파일헤더의 recordnum,pagenum값이 current->pagenum과 같은경우)
					if (readFileHeader(fp, fileHeaderBuf) != 1)
					{
						fprintf(stderr, "ERROR: Failed to read file header\n");
						fclose(fp);
						exit(1);
					}

					memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));
					memcpy(&delete_list_front_pagenum, fileHeaderBuf + sizeof(short), sizeof(short));
					memcpy(&delete_list_front_recordnum, fileHeaderBuf + 2 * sizeof(short), sizeof(short));

					// 내일할일 : 삭제된 레코드에 삽입하고 -1,-1로 채워야하는 경우 안되는 문제
					if (current->prev == NULL)
					{

						delete_list_front_pagenum = -1;
						delete_list_front_recordnum = -1;
					}
					else
					{

						delete_list_front_pagenum = current->prev->page_num;
						delete_list_front_recordnum = current->prev->record_num;
					}
					int finPage = delete_list_front_pagenum;
					int finRec = delete_list_front_recordnum;

					memcpy(fileHeaderBuf, &header_total_pages, sizeof(short));
					memcpy(fileHeaderBuf + sizeof(short), &delete_list_front_pagenum, sizeof(short));
					memcpy(fileHeaderBuf + 2 * sizeof(short), &delete_list_front_recordnum, sizeof(short));

					if (writeFileHeader(fp, fileHeaderBuf) != 1)
					{
						fprintf(stderr, "ERROR: Failed to write file header\n");
						fclose(fp);
						exit(1);
					}
					// 삭제 리스트에서는 해당 노드를 삭제한다.
					deleteDeleteNode(&deleteHead, &deleteTail, pagenum, recordnum);

					break;
				}

				current = current->prev;
			}
		}
		else
		{
			memset(pagebuf, 0, PAGE_SIZE);
			// 마지막 페이지 읽기
			// 파일 헤더 읽기
			memset(fileHeaderBuf, 0, FILE_HEADER_SIZE);
			if (readFileHeader(fp, fileHeaderBuf) != 1)
			{
				fprintf(stderr, "ERROR: Failed to read file header\n");
				fclose(fp);
				exit(1);
			}

			memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));

			if (readPage(fp, pagebuf, header_total_pages) != 1)
			{
				fprintf(stderr, "ERROR: read page error\n");
				exit(1);
			}

			memset(recordbuf, 0, MAX_RECORD_SIZE);
			pack(recordbuf, s);

			// writeRecToPagebuf(fp, header_total_pages, pagebuf, recordbuf);
			int pagenum = header_total_pages;
			short total_records; // 페이지에 저장된 총 레코드 수
			short freespace;	 // 페이지의 여유 공간
			int reserved_space;	 // 예약된 공간
			// 페이지의 총 레코드 수와 여유 공간 읽기
			memcpy(&total_records, pagebuf, sizeof(short));
			memcpy(&freespace, pagebuf + sizeof(short), sizeof(short));
			memcpy(&reserved_space, pagebuf + 2 * sizeof(short), sizeof(int));

			short record_length = strlen(recordbuf); // 레코드의 길이

			// 3-1) 레코드가 현재 페이지의 여유 공간보다 큰 경우, 새로운 페이지에 저장
			if (record_length + sizeof(short) > freespace)
			{

				// 파일 헤더의 총 페이지 수를 업데이트
				pagenum++;

				// 새로운 페이지 초기화
				memset(pagebuf, 0, PAGE_SIZE);
				total_records = 0;
				freespace = PAGE_SIZE - PAGE_HEADER_SIZE;
				reserved_space = 0;

				memcpy(pagebuf, &total_records, sizeof(short));
				memcpy(pagebuf + sizeof(short), &freespace, sizeof(short));
				memcpy(pagebuf + 2 * sizeof(short), &reserved_space, sizeof(int));

				// 파일 헤더 업데이트
				char fileHeaderBuf[FILE_HEADER_SIZE];
				short header_total_pages;

				if (readFileHeader(fp, fileHeaderBuf) != 1)
				{
					fprintf(stderr, "ERROR: Failed to read file header\n");
					exit(1);
				}

				memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));
				header_total_pages++;
				memcpy(fileHeaderBuf, &header_total_pages, sizeof(short));

				if (writeFileHeader(fp, fileHeaderBuf) != 1)
				{
					fprintf(stderr, "ERROR: Failed to write file header\n");
					exit(1);
				}

				writeRecToPagebuf(pagebuf, recordbuf);

				// 현재 페이지를 파일에 기록
				if (writePage(fp, pagebuf, pagenum) != 1)
				{
					fprintf(stderr, "ERROR: Failed to write page\n");
					exit(1);
				}
			}
			else
			{
				writeRecToPagebuf(pagebuf, recordbuf);
				// 현재 페이지를 파일에 기록
				if (writePage(fp, pagebuf, pagenum) != 1)
				{
					fprintf(stderr, "ERROR: Failed to write page\n");
					exit(1);
				}
			}
		}
	}
}

// 레코드를 페이지 버퍼에 기록하는 함수 -> writePage까지 함께
void writeRecToPagebuf(char *pagebuf, const char *recordbuf)
{
	short total_records; // 페이지에 저장된 총 레코드 수
	short freespace;	 // 페이지의 여유 공간
	int reserved_space;	 // 예약된 공간

	// 페이지의 총 레코드 수와 여유 공간 읽기
	memcpy(&total_records, pagebuf, sizeof(short));
	memcpy(&freespace, pagebuf + sizeof(short), sizeof(short));
	memcpy(&reserved_space, pagebuf + 2 * sizeof(short), sizeof(int));

	short record_length = strlen(recordbuf); // 레코드의 길이

	// 3-2) free space에 레코드가 들어갈 수 있는 경우 pagebuf에 저장
	short offset;
	if (total_records == 0)
	{
		// 첫 번째 레코드인 경우
		offset = PAGE_HEADER_SIZE;
	}
	else
	{
		// 이전 레코드의 끝 위치를 가져와서 새로운 레코드의 시작 위치를 계산
		short last_record_end = PAGE_HEADER_SIZE - 1;

		memcpy(&last_record_end, pagebuf + 8 + (total_records - 1) * sizeof(short), sizeof(short));

		offset = last_record_end + 1;
	}

	// 레코드 저장
	strncpy(pagebuf + offset, recordbuf, record_length);

	// 오프셋 업데이트 (마지막 바이트 주소를 기록)
	short record_end = offset + record_length - 1;
	memcpy(pagebuf + 8 + total_records * sizeof(short), &record_end, sizeof(short));

	// 헤더 업데이트
	total_records++;			// 총 레코드 수 증가
	freespace -= record_length; // 여유 공간 감소

	memcpy(pagebuf, &total_records, sizeof(short));
	memcpy(pagebuf + sizeof(short), &freespace, sizeof(short));
	memcpy(pagebuf + 2 * sizeof(short), &reserved_space, sizeof(int));
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
	short header_total_pages;
	memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));

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

		short numRecords;
		memcpy(&numRecords, pagebuf, sizeof(short)); // 페이지 헤더 첫 번째 2바이트에서 총 레코드 수 읽기

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

// @todo
int searchByID(FILE *fp, char *keyval, char *recordbuf, int *pagenum, int *recordnum)
{

	char fileHeaderBuf[FILE_HEADER_SIZE];
	char pagebuf[PAGE_SIZE];
	STUDENT student;
	// 레코드 파일 헤더 가져오기
	if (readFileHeader(fp, fileHeaderBuf) != 1)
	{
		fprintf(stderr, "ERROR: Failed to write file header\n");
		fclose(fp);
		exit(1);
	}
	short header_total_pages;
	memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));

	int match = 0;
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

		short numRecords;
		memcpy(&numRecords, pagebuf, sizeof(short)); // 페이지 헤더 첫 번째 2바이트에서 총 레코드 수 읽기

		for (int j = 0; j < numRecords; j++)
		{
			getRecFromPagebuf(pagebuf, recordbuf, j);

			unpack(recordbuf, &student);

			if (strcmp(student.id, keyval) == 0)
			{
				match = 1;
				*pagenum = i;
				*recordnum = j;
				return 1;
			}
		}
	}
	return match;
}

// @todo
// keyval은 학번임
void delete(FILE *fp, char *keyval)
{
	char recordbuf[MAX_RECORD_SIZE];
	char fileHeaderBuf[FILE_HEADER_SIZE];
	char pagebuf[PAGE_SIZE];
	int pagenum;
	int recordnum;
	char recordStoreBuf[MAX_RECORD_SIZE];

	// 해당 id가 존재하면
	if (searchByID(fp, keyval, recordbuf, &pagenum, &recordnum) == 1)
	{
		strcpy(recordStoreBuf, recordbuf);
		if (readFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write file header\n");
			fclose(fp);
			exit(1);
		}

		short header_total_pages;
		short delete_list_front_pagenum;
		short delete_list_front_recordnum;
		// 파일 헤더의 데이터들 가져오기
		memcpy(&header_total_pages, fileHeaderBuf, sizeof(short));
		memcpy(&delete_list_front_pagenum, fileHeaderBuf + sizeof(short), sizeof(short));
		memcpy(&delete_list_front_recordnum, fileHeaderBuf + 2 * sizeof(short), sizeof(short));

		// 레코드 업데이트
		char star = '*';

		memcpy(recordbuf, &star, sizeof(char));
		memcpy(recordbuf + 1, &delete_list_front_pagenum, sizeof(short));
		memcpy(recordbuf + 1 + sizeof(short), &delete_list_front_recordnum, sizeof(short));

		// 해당 pagenum에 해당하는 페이지를 pagebuf에 담아온다.
		if (readPage(fp, pagebuf, pagenum) == 0)
		{
			fprintf(stderr, "ERROR1: page read error\n");
			exit(1);
		}

		// pagebuf에 recordnum이 해당하는 offset위치에 record 덮어 쓴다.
		short offset;
		short next_offset;

		// 첫 번째 레코드의 경우, 시작 오프셋을 PAGE_HEADER_SIZE로 설정
		if (recordnum == 0)
		{
			offset = PAGE_HEADER_SIZE;
		}
		else
		{
			// 현재 레코드의 시작위치 알기 위해 이전 레코드의 끝 오프셋을 가져옴
			memcpy(&offset, pagebuf + 8 + (recordnum - 1) * sizeof(short), sizeof(short));
			// 시작 오프셋을 계산하기 위해 1을 더함
			offset++;
		}

		// 현재 레코드의 끝 오프셋을 가져옴
		memcpy(&next_offset, pagebuf + 8 + recordnum * sizeof(short), sizeof(short));
		// 레코드의 길이를 계산
		int length = next_offset - offset + 1;

		// 업데이트된 recordbuf를 페이지 버퍼에 복사
		memcpy(pagebuf + offset, recordbuf, length);

		if (writePage(fp, pagebuf, pagenum) != 1)
		{
			fprintf(stderr, "Error: write page error\n");
			exit(1);
		}

		// 파일 헤더 업데이트
		delete_list_front_pagenum = pagenum;
		delete_list_front_recordnum = recordnum;

		memcpy(fileHeaderBuf + sizeof(short), &delete_list_front_pagenum, sizeof(short));
		memcpy(fileHeaderBuf + 2 * sizeof(short), &delete_list_front_recordnum, sizeof(short));

		if (writeFileHeader(fp, fileHeaderBuf) != 1)
		{
			fprintf(stderr, "ERROR: Failed to write file header\n");
			fclose(fp);
			exit(1);
		}

		STUDENT student;
		memset(&student, 0, sizeof(STUDENT));

		unpack(recordStoreBuf, &student);

		printSearchResult(&student, 1);
	}
	else
	{
		printf("#Records = 0\n");
	}
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

	while ((opt = getopt(argc, argv, "i:s:d:")) != -1)
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
		else if (opt == 'd')
		{
			if ((fp = fopen(fileName, "rb+")) < 0)
			{
				fprintf(stderr, "ERROR: fopen error\n");
				exit(1);
			}
			char *field = strtok(argv[3], "=");
			char *value = strtok(NULL, "=");
			delete (fp, value);
		}
	}

	exit(0);
}
