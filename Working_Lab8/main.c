#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

typedef struct Filter{
    int size;
    double** matrix;
} Filter;

typedef struct Image{
    int width;
    int height;
    int** matrix;
} Image;

Filter *filterMatrix;
Image *imageMatrix;
Image *outputMatrix;
int threadCount;
char *mode;
char *imageFilename, *filterFilename, *outputFilename;

void die_errno(char *message) {
    printf("ERROR: %s\n", message);
    exit(1);
}

void getFilterMatrix(FILE *filterFile) {
    if (filterFile == NULL) die_errno("Couldn't open filterFile");
    int size;
    filterMatrix = malloc(sizeof(Filter));
    fscanf(filterFile, "%i", &size);
    filterMatrix->size = size;
    filterMatrix->matrix = malloc(filterMatrix->size * sizeof(double *));
    for (int i = 0; i < filterMatrix->size; i++)
        filterMatrix->matrix[i] = malloc(filterMatrix->size * sizeof(double));

    for (int x = 0; x < filterMatrix->size; x++) {
        for (int y = 0; y < filterMatrix->size; y++) {
            fscanf(filterFile, "%lf", &filterMatrix->matrix[x][y]);
        }
    }

    fclose(filterFile);
}

void getImageMatrix(FILE *imageFile) {
    int width, height;
    if (imageFile == NULL) die_errno("Couldn't open imageFile");
    fscanf(imageFile, "P2 %i %i 255", &width, &height);
    imageMatrix = malloc(sizeof(imageMatrix));
    imageMatrix->width = width;
    imageMatrix->height = height;

    imageMatrix->matrix = malloc(imageMatrix->height * sizeof(int *));
    for (int i = 0; i < imageMatrix->height; i++) {
        imageMatrix->matrix[i] = malloc(imageMatrix->width * sizeof(int));
    }

    for (int y = 0; y < imageMatrix->height; y++) {
        for (int x = 0; x < imageMatrix->width; x++) {
            fscanf(imageFile, "%i", &imageMatrix->matrix[y][x]);
        }
    }

    fclose(imageFile);
}

void getOutMatrix() {
    outputMatrix = malloc(sizeof(outputMatrix));
    outputMatrix->height = imageMatrix->height;
    outputMatrix->width = imageMatrix->width;
    outputMatrix->matrix = malloc(outputMatrix->height * sizeof(int *));
    for (int i = 0; i < outputMatrix->height; i++) {
        outputMatrix->matrix[i] = malloc(outputMatrix->width * sizeof(int));
        for (int j = 0; j < outputMatrix->width; j++) {
            outputMatrix->matrix[i][j] = 0;
        }
    }
}

int max(int first, int second) {
    return first > second ? first : second;
}

int min(int first, int second){
    return first > second ? second : first;
}

void filter(int width, int height) {
    double pixel = 0;
    for (int k = 0; k < filterMatrix->size; k++) {
        for (int l = 0; l < filterMatrix->size; l++) {
            int half = ceil(0.5 * filterMatrix->size);
            int CY = max(0, height - half + l - 1);
            int GY = min(CY, imageMatrix->height - 1);
            int CX = max(0, width - half + k - 1);
            int GX = min(CX, imageMatrix->width - 1);
            int processedPixel = imageMatrix->matrix[GY][GX];
            pixel += filterMatrix->matrix[k][l] * processedPixel;
        }
    }
    outputMatrix->matrix[height][width] = round(pixel);
}

long getCurrentTime() {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * (int) 1e6 + currentTime.tv_usec;
}

void *blockMode(void *id) {
    long start = getCurrentTime();
    int current = (*(int *) id);
    int sector = (int) ceil(1.0 * imageMatrix->width / threadCount);
    for (int width = current * sector; width < (current + 1) * sector && width < imageMatrix->width; width++) {
        for (int height = 0; height < imageMatrix->height; height++) {
            filter(width, height);
        }
    }

    long *tmp = malloc(sizeof(long));
    *tmp = getCurrentTime() - start;
    pthread_exit(tmp);
}

void *interleavedMode(void *id) {
    long start = getCurrentTime();
    int thisId = (*(int *) id);
    for (int width = thisId; width < imageMatrix->width; width += threadCount) {
        for (int height = 0; height < imageMatrix->height; height++) {
            filter(width, height);
        }
    }
    long *tmp = malloc(sizeof(long));
    *tmp = getCurrentTime() - start;
    pthread_exit(tmp);
}

void createOutImage(FILE *outFile) {
    if (outFile == NULL) die_errno("Couldn't open outFile");
    fprintf(outFile, "P2\n%i %i\n255\n", outputMatrix->width, outputMatrix->height);
    for (int y = 0; y < outputMatrix->height; y++) {
        for (int x = 0; x < outputMatrix->width; x++) {
            fprintf(outFile, "%i", (outputMatrix->matrix[y][x] + 256) % 256);
            if (x + 1 != outputMatrix->width)
                fputc(' ', outFile);
        }
        fputc('\n', outFile);
    }
    fclose(outFile);
}

void parse_input(int argc, char **argv){
    if (argc != 6) die_errno("Wrong num of arguments");

    threadCount = atoi(argv[1]);
    mode = argv[2];
    if(strcmp(mode, "block") != 0 && strcmp(mode, "interleaved") != 0)
        die_errno("Wrong mode");
    imageFilename = argv[3];
    filterFilename = argv[4];
    outputFilename = argv[5];

    printf("THREAD COUNT: %d, ", threadCount);
    printf("MODE: %s, ", mode);
    printf("FILTER NAME: %s, ", filterFilename);
    printf("IMAGE FILE: %s, ", imageFilename);
    printf("OUTPUT FILE: %s.\n", outputFilename);
}

int main(int argc, char **argv) {
    long start = getCurrentTime();

    parse_input(argc, argv);

    FILE *imageFile = fopen(imageFilename, "r");
    getImageMatrix(imageFile);

    FILE *filterFile = fopen(filterFilename, "r");
    getFilterMatrix(filterFile);

    getOutMatrix();

    pthread_t threads[threadCount];

    for (int i = 0; i < threadCount; i++) {
        int *val = malloc(sizeof(int));
        *val = i;
        pthread_t thread;
        if (!strcmp(mode, "block")) pthread_create(&thread, NULL, &blockMode, val);
        else pthread_create(&thread, NULL, &interleavedMode, val);
        threads[i] = thread;
    }

    printf("I-th thread took ... \n");
    for (int i = 0; i < threadCount; i++) {
        long *time;
        pthread_join(threads[i], (void *) &time);
        printf(" %i: %ld |", i, *time);
    }
    printf(" ... time.\n");

    FILE *outputFile = fopen(outputFilename, "w");
    createOutImage(outputFile);

    printf("Total time: %ld\n\n", getCurrentTime() - start);
    printf("-----------------------------------------------------------\n");

    return 0;
}