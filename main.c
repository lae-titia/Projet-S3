
int main(int argc,char** const argv)
{
	if (argc != 3)
	{
		errx(2,"wrong number of arguments");
	}
	char* fileName = argv[1];
	char* word = argv[2];
	solver(fileName,word);
	return 0;
}
