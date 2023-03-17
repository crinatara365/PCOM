#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char* get_cookie(char *response) {
    char *p = strstr(response, "connect.");
    const char delim[] = ";";
    char *cookie = strtok(p, delim);  
    return cookie;
}

char* get_token(char *response) {
    char *p1 = strstr(response, "{");
    char *p2 = strstr(p1, ":");
    p2 += 2;
    const char delim[] = "}";
    char *token = strtok(p2, delim);
    token[strlen(token)-1] = '\0';
    return token;
}

char* register_response(char *response) {
    char *message = NULL;
    char *p = strstr(response, "Created");
    if(p == NULL) {
        message = "Username indisponibil";
    }
    else
        message = "Utilizator inregistrat cu succes";
    return message;
}

int login_response(char *response) {
    char *aux = response;
    char *p = strstr(response, "OK");
    if(p == NULL) {
        if(strstr(aux, "account") != NULL) 
            printf("Nu exista cont cu acest username\n");
        else 
            printf("Credentiale incorecte\n");
        return 1;
    }
    else {
        printf("Utilizator logat cu succes\n");
        return 0;
    }
}

int enter_response(char *response) {
    char *p = strstr(response, "OK");
    if(p == NULL) {
        printf("Cerere de acces respinsa: nu sunteti autentificat\n");
        return 1;
    }
    else {
        printf("Cerere de acces acceptata - Bine ati venit!\n");
        return 0;
    }
}

char* books_response(char *response) {
    char *message = NULL;
    char *p = strstr(response, "OK");
    if(p == NULL) {
        message = "Utilizator fara acces la biblioteca";
    }
    else
        message = strstr(p, "[");
    return message;
}

char* book_response(char *response) {
    char *message = NULL;
    char *aux = response;
    char *p = strstr(response, "OK");
    if(p == NULL) {
        if(strstr(aux, "decoding") != NULL)
            message = "Utilizator fara acces la biblioteca";
        else    
            message = "Nu exista carte cu id-ul dat";
    }
    else
        message = strstr(p, "[");
    return message;
}

char* add_response(char *response) {
    char *message = NULL;
    char *p = strstr(response, "OK");
    if(p == NULL) 
        message = "Utilizator fara acces la biblioteca";
    else
        message = "Carte adaugata cu succes";
    return message;
}

char* delete_response(char *response) {
    char *message = NULL;
    char *aux = response;
    char *p = strstr(response, "OK");
    if(p == NULL) 
        if(strstr(aux, "decoding") != 0)
            message = "Utilizator fara acces la biblioteca";
        else
            message = "Cartea cu id-ul dat nu exista";    
    else
        message = "Carte stearsa cu succes";
    return message;
}

int logout_response(char *response) {
    char *p = strstr(response, "OK");
    if(p == NULL) {
        printf("Utilizator neautentificat\r\n");
        return 1;
    }
    else {
        printf("Utilizator delogat cu succes\r\n");
        return 0;
    }
}

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    char **cookies = calloc(1, sizeof(char *));
    cookies[0] = calloc(LINELEN, sizeof(char));
    char *token = NULL;

    while (1) {
        sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);
        char command[20];
        scanf("%s", command);

        if(strcmp(command, "register") == 0) {
            char username[100], password[100];
            printf("username=");
            scanf("%s", username);
            printf("password=");
            scanf("%s", password);
            char **form_data = calloc(1, sizeof(char *));
            form_data[0] = calloc(LINELEN, sizeof(char));
            sprintf(form_data[0], "{\n\t\"username\": \"%s\", \n\t\"password\": \"%s\"\n}", username, password);
            message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/register", "application/json", NULL, form_data, 1, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(register_response(response));
        }

        if(strcmp(command, "login") == 0) {
            /* serverul intoarce cod OK deci verificam in client daca user deja logat */
            if(*cookies[0] != '\0')
                printf("Utilizator deja logat\n");
            else {
                char username[100], password[100];
                printf("username=");
                scanf("%s", username);
                printf("password=");
                scanf("%s", password);
                char **form_data = calloc(1, sizeof(char *));
                form_data[0] = calloc(LINELEN, sizeof(char));
                sprintf(form_data[0], "{\n\t\"username\": \"%s\", \n\t\"password\": \"%s\"\n}", username, password);
                message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/login", "application/json", NULL, form_data, 1, NULL, 0);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                if(login_response(response) == 0)
                    sprintf(cookies[0], "%s", get_cookie(response));
            }
        }

        if(strcmp(command, "enter_library") == 0) {
            message = compute_get_request("34.241.4.235", "/api/v1/tema/library/access", NULL, NULL, cookies, 1);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if(enter_response(response) == 0)
                token = get_token(response);
        }

        if(strcmp(command, "get_books") == 0) {
            char *header = malloc(1000);
            sprintf(header, "Authorization: Bearer %s", token);
            message = compute_get_request("34.241.4.235", "/api/v1/tema/library/books", NULL, header, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(books_response(response));
        }

        if(strcmp(command, "get_book") == 0) {
            int id;
            printf("id=");
            scanf("%d", &id);
            char *url = malloc(100);
            sprintf(url, "/api/v1/tema/library/books/%d", id);
            char *header = malloc(1000);
            sprintf(header, "Authorization: Bearer %s", token);
            message = compute_get_request("34.241.4.235", url, NULL, header, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(book_response(response));
        }

        if(strcmp(command, "add_book") == 0) {
            char title[100], author[100], genre[100], publisher[100], page_count[100];
            char temp;

            printf("title=");
            scanf("%c",&temp);
            scanf("%[^\n]", title);

            printf("author=");
            scanf("%c",&temp);
            scanf("%[^\n]", author);

            printf("genre=");
            scanf("%c",&temp);
            scanf("%[^\n]", genre);

            printf("publisher=");
            scanf("%c",&temp);
            scanf("%[^\n]", publisher);
           
            printf("page_count=");
            scanf("%c",&temp);
            scanf("%[^\n]", page_count);

            /* se verifica daca page_count este valid */
            int contor = 0;
            if(page_count[0] >= '1' && page_count[0] <= '9')
                for(int i = 1; i < strlen(page_count); i++) {
                    if(page_count[i] >= '0' && page_count[i] <= '9')
                        contor = 0;
                    else {
                        contor = 1;
                        break;
                    }   
                }
            else
                contor = 1;

            if(contor == 0) {
                char *header = malloc(1000);
                sprintf(header, "Authorization: Bearer %s", token);
                char **form_data = calloc(1, sizeof(char *));
                form_data[0] = calloc(LINELEN, sizeof(char));
                sprintf(form_data[0], "{\n\t\"title\": \"%s\", \n\t\"author\": \"%s\", \n\t\"genre\": \"%s\", \n\t\"page_count\": %s, \n\t\"publisher\": \"%s\"\n}", title, author, genre, page_count, publisher);  
                message = compute_post_request("34.241.4.235", "/api/v1/tema/library/books", "application/json", header, form_data, 1, NULL, 0);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                puts(add_response(response));
            }
            else
                printf("Date invalide\n");
        }

        if(strcmp(command, "delete_book") == 0) {
            int id;
            printf("id=");
            scanf("%d", &id);
            char *url = malloc(100);
            sprintf(url, "/api/v1/tema/library/books/%d", id);
            char *header = malloc(1000);
            sprintf(header, "Authorization: Bearer %s", token);
            message = compute_delete_request("34.241.4.235", url, header);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(delete_response(response));
        }

        if(strcmp(command, "logout") == 0) {
            message = compute_get_request("34.241.4.235", "/api/v1/tema/auth/logout", NULL, NULL, cookies, 1);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if(logout_response(response) == 0) {
                *cookies[0] = '\0';
                token = NULL;
            }
        }

        if(strcmp(command, "exit") == 0) {
            break;
        }
    }

    close(sockfd);
    free(message);
    free(response);

    return 0;
}
