#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_END 0
#define TYPE_PIPE 1
#define TYPE_BREAK 2

typedef struct s_list {
	char		**args;
	int 		len;
	int 		type;
	int			pipes[2];
	struct s_list *next;
	struct s_list *last;
}				t_list;

int		ft_strlen(const char *str)
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

char	*ft_strdup(char *str)
{
	char *copy;
	int i = 0;

	copy = malloc(ft_strlen(str) * sizeof(char));
	if (!copy)
		return (NULL);
	while (str[i] != '\0')
	{
		copy[i] = str[i];
		i++;
	}
	copy[i] = '\0';
	return (copy);
}

int 	add_arg(t_list *cmd, char *str)
{
	char **tmp;
	int i = 0;

	tmp = NULL;
	tmp = malloc(sizeof(*tmp) * (cmd->len + 2));
	if (!tmp)
		return (exit_fatal());
	while (i < cmd->len)
	{
		tmp[i] = cmd->args[i];
		i++;
	}
	if (cmd->len > 0)
		free(cmd->args);
	cmd->args = tmp;
	cmd->args[i++] = ft_strdup(str);
	cmd->args[i] = 0;
	cmd->len++;
	return (EXIT_SUCCESS);
}

int		list_push(t_list **cmds, char *str)
{
	t_list *new;

	new = malloc(sizeof(t_list));
	if (!new)
		return (exit_fatal());
	new->args = NULL;
	new->last = NULL;
	new->type = TYPE_END;
	new->next = NULL;
	new->len = 0;
	if (*cmds)
	{
		(*cmds)->next = new;
		new->last = *cmds;
	}
	*cmds = new;
	return (add_arg(new, str));
}

int		parse_arg(t_list **cmds, char *str)
{
	int		is_break;

	is_break = (strcmp(";", str) == 0);
	if (is_break && !*cmds)
		return (EXIT_SUCCESS);
	else if (!is_break && (!*cmds || (*cmds)->type > TYPE_END))
		return (list_push(cmds, str));
	else if (strcmp("|", str) == 0)
		(*cmds)->type = TYPE_PIPE;
	else if (is_break)
		(*cmds)->type = TYPE_BREAK;
	else
		return (add_arg(*cmds, str));
	return (EXIT_SUCCESS);
}

int		list_rewind(t_list **list)
{
	while (*list && (*list)->last)
		*list = (*list)->last;
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
		if ((ret = execve(cmd->args[0], cmd->args, env)) < 0)
		{
			show_error("error: cannot execute ");
			show_error(cmd->args[0]);
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

int		exec_cmds(t_list **cmds, char **env)
{
	t_list *crt;
	int ret;

	ret = EXIT_SUCCESS;
	list_rewind(cmds);
	while (*cmds)
	{
		crt = *cmds;
		if (strcmp("cd", crt->args[0]) == 0)
		{
			ret = EXIT_SUCCESS;
			if (crt->len < 2)
				ret = show_error("error: cd: bad arguments\n");
			else if (chdir(crt->args[1]))
			{
				ret = show_error("error: cd: cannot change directory to ");
				show_error(crt->args[1]);
				show_error("\n");
			}
		}
		else
			exec_cmd(crt, env);
		if (!(*cmds)->next)
			break ;
		*cmds = (*cmds)->next;
	}
	return (ret);
}

int		main(int argc, char **argv, char **env) {

	t_list *cmds;
	int i = 1;
	int res;

	cmds = NULL;
	res = EXIT_SUCCESS;

	while (i < argc)
		parse_arg(&cmds, argv[i++]);
	if (cmds)
		res = exec_cmds(&cmds, env);
	return (res);
}
