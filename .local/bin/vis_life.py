#!/usr/bin/python

import calplot
import os
import numpy as np; np.random.seed(sum(map(ord, 'calplot')))
import pandas as pd
import matplotlib.pyplot as plt

import pytodotxt

def main():
    todotxt = pytodotxt.TodoTxt(os.path.expanduser('~')+'/todo/done.txt')
    todotxt.parse()

    completed = []
    descriptions = []

    for task in todotxt.tasks:
        if task.is_completed:
            completed.append(str(task.completion_date))
            descriptions.append(task.description)

    # Convert to isoformat that calplot wants using pandas
    completed = pd.DatetimeIndex(completed)

    df = pd.DataFrame(list(zip(completed, descriptions)), 
                      columns = ['completion_date', 'description'])

    # Count how many tasks have been done each day and store in pandas Series
    events = df.pivot_table(index = ['completion_date'], aggfunc ='size')

    # # Example of events generated randomly
    # all_days = pd.date_range('1/1/2023', periods=365, freq='D')
    # days = np.random.choice(all_days, 1)
    # events = pd.Series(np.random.randn(len(days)), index=days)

    calplot.calplot(events, vmin=-1, vmax=7, colorbar=True, linewidth=2, cmap='YlGn')

    plt.savefig('/tmp/done.png', bbox_inches='tight')

    # plt.show()

if __name__ == "__main__":
    main()
