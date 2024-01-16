create type log_level as enum ('debug', 'info', 'notice', 'warning', 'error', 'critical', 'alert', 'emergency');

create table log (
    log_id      bigserial not null primary key,
    level       log_level not null,
    message     text      not null,
    created_at  timestamp not null default current_timestamp
);

create table task (
    task_id        bigserial not null primary key,
    input_path     text      not null,
    output_path    text      not null,
    priority       int       not null default 0,
    custom_data_1  text          null default null,
    custom_data_2  bigint        null default null,
    settings_csv   text          null default null,
    created_at     timestamp not null default current_timestamp,

    constraint unique_task_input_path  unique (input_path),
    constraint unique_task_output_path unique (output_path)
);

create index index_task_custom_data_1 on task (custom_data_1);
create index index_task_custom_data_2 on task (custom_data_2);
