#define TYPE_END 0
#define TYPE_PIPE 1
#define TYPE_BREAK 2

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct s_list {
	int pipes[2];
	int len;
	int type;
	char **argv;
	struct s_list *next;
	struct s_list *last;

}				t_list;


int		ft_strlen(char *str)
{
	int i = 0;

	while (str[i] != '\0')
		i++;
	return (i);
}

int 	show_error(char *str)
{
	if (str)
		write(2, str, ft_strlen(str));
	return (EXIT_FAILURE);
}

int		exit_fatal()
{
	show_error("error: falal\n");
	exit(EXIT_FAILURE);
	return (EXIT_FAILURE);
}

char *ft_strdup(char *str)
{
	char *tmp;
	int i = 0;

	tmp = malloc(sizeof(char) * ft_strlen(str));
	while (str[i] != '\0')
	{
		tmp[i] = str[i];
		i++;
	}
	tmp[i] = '\0';

	return (tmp);
}

int		add_arg(t_list *cmds, char *str)
{
	char **tmp;
	int i = 0;

	tmp = malloc(sizeof(*tmp) * (cmds->len + 2));

	while (i < cmds->len)
	{
		tmp[i] = cmds->argv[i];
		i++;
	}
	if (cmds->len > 0)
		free(cmds->argv);
	cmds->argv = tmp;
	cmds->argv[i++] = ft_strdup(str);
	cmds->argv[i] = NULL;
	cmds->len++;
	return (EXIT_SUCCESS);
}

int		list_push(t_list **cmds, char *str)
{
	t_list *tmp;

	tmp = malloc(sizeof(t_list));

	tmp->type = TYPE_END;
	tmp->argv = NULL;
	tmp->last = NULL;
	tmp->next = NULL;
	tmp->len = 0;
	if (*cmds)
	{
		(*cmds)->next = tmp;
		tmp->last = *cmds;
	}
	*cmds = tmp;
	return (add_arg(tmp, str));
}

int 	parse_arg(t_list **cmds, char *str)
{
	int is_break;

	is_break = (strcmp(";", str) == 0);
	if (is_break && !*cmds)
		return (EXIT_SUCCESS);
	else if (!is_break && (!*cmds || (*cmds)->type > TYPE_END))
		list_push(cmds, str);
	else if (strcmp("|", str) == 0)
		(*cmds)->type = TYPE_PIPE;
	else if (is_break)
		(*cmds)->type = TYPE_BREAK;
	else
		add_arg(*cmds, str);
	return (EXIT_SUCCESS);
}

int		exec_cmd(t_list *cmd, char **env)
{
	pid_t 	pid;
	int		ret;
	int		status;
	int		pipe_open;

	ret = EXIT_FAILURE;
	pipe_open = 0;
	if (cmd->type == TYPE_PIPE || (cmd->last && cmd->last->type == TYPE_PIPE))
	{
		pipe_open = 1;
		if (pipe(cmd->pipes))
			return (exit_fatal());
	}
	pid = fork();
	if (pid < 0)
		return (exit_fatal());
	else if (pid == 0)
	{
		if (cmd->type == TYPE_PIPE && dup2(cmd->pipes[0], 1) < 0)
			return (exit_fatal());
		if (cmd->last && cmd->last->type == TYPE_PIPE && dup2(cmd->pipes[1], 0) < 0)
			return (exit_fatal());
		if ((ret = execve(cmd->argv[0], cmd->argv, env)) < 0)
		{
			show_error("error: cannot execute ");
			show_error(cmd->argv[0]);
			show_error("\n");
		}
		exit(ret);
	}
	else
	{
		waitpid(pid, &status, 0);
		if (pipe_open)
		{
			close(cmd->pipes[0]);
			if (!cmd->next || cmd->type == TYPE_BREAK)
				close(cmd->pipes[1]);
		}
		if (cmd->last && cmd->last->type == TYPE_PIPE)
			close(cmd->last->pipes[1]);
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);
	}
	return (ret);
}

int		list_rewind(t_list **list)
{
	while (*list && (*list)->last)
		*list = (*list)->last;
	return (EXIT_SUCCESS);
}


int		list_clear(t_list **cmds)
{
	t_list *tmp;
	int i;

	list_rewind(cmds);
	while (*cmds)
	{
		tmp	= (*cmds)->next;
		i = 0;

		while ((*cmds)->len > i)
			free((*cmds)->argv[i++]);
		free((*cmds)->argv);
		free(*cmds);

		(*cmds) = tmp;
	}
	*cmds = NULL;
	return (EXIT_SUCCESS);
}

int		exec_cmds(t_list **cmds, char **env)
{
	int res;
	t_list *crt;

	res = EXIT_SUCCESS;
	list_rewind(cmds);
	while (*cmds)
	{
		crt = *cmds;
		if (strcmp("cd", crt->argv[0]) == 0)
		{
			res = EXIT_SUCCESS;
			if (crt->len < 2)
				res = show_error("");
			else if (chdir(crt->argv[1]))
				res = show_error("");
		}
		else
			exec_cmd(crt, env);
		if (!(*cmds)->next)
			break;
		*cmds = (*cmds)->next;
	}

	return  (res);
}

int main(int argc, char **argv, char **env)
{
	t_list *cmds;
	int i = 1;
	int res = EXIT_SUCCESS;

	cmds = NULL;
	while (i < argc)
		parse_arg(&cmds, argv[i++]);
	if (cmds)
		res = exec_cmds(&cmds, env);
	list_clear(&cmds);
	return (res);
}
